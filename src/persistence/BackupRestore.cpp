#include "lms/persistence/BackupManager.h"
#include <sodium.h>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace lms {
namespace {
namespace fs=std::filesystem;
std::string nonce() {
    unsigned char bytes[8]{};
    char hex[17]{};
    randombytes_buf(bytes,sizeof(bytes));
    sodium_bin2hex(hex,sizeof(hex),bytes,sizeof(bytes));
    return hex;
}
bool nested(fs::path child,fs::path ancestor) {
    child=fs::weakly_canonical(child);
    ancestor=fs::weakly_canonical(ancestor);
    auto i=child.begin();
    for(auto a=ancestor.begin();a!=ancestor.end();++a,++i)
        if(i==child.end()||*i!=*a)return false;
    return true;
}
void copyAttachments(const fs::path& source,const fs::path& dest) {
    fs::create_directories(dest);
    for(const auto& item:fs::directory_iterator(source)) {
        if(fs::symlink_status(item.path()).type()!=fs::file_type::regular)
            throw std::runtime_error("Unexpected attachment type");
        fs::copy_file(item.path(),dest/item.path().filename());
    }
}
} // namespace

fs::path BackupManager::restore(const fs::path& backup,const fs::path& data)
{
    // Caller must acquire lms.lock (the same lock as the regular LMS).
    verify(backup);
    if(fs::symlink_status(data).type()!=fs::file_type::directory ||
       fs::symlink_status(data/"lms.sqlite3").type()!=fs::file_type::regular)
        throw std::runtime_error("Existing LMS data is missing; restore aborted");
    if(nested(backup,data))
        throw std::invalid_argument("Backup cannot be located inside live application data");

    // Create a safety backup of the CURRENT dataset before replacement.
    auto safetyRoot=data.parent_path()/"ModeratedLMS_PreRestore_Safety";
    fs::create_directories(safetyRoot);
    sqlite3* original=nullptr;
    if(sqlite3_open_v2((data/"lms.sqlite3").string().c_str(),&original,
                       SQLITE_OPEN_READONLY,nullptr)!=SQLITE_OK) {
        if(original)sqlite3_close(original);
        throw std::runtime_error("Cannot open existing database for safety backup");
    }
    fs::path safety;
    try { safety=create(original,data,safetyRoot).folder; }
    catch(...) {sqlite3_close(original);throw;}
    sqlite3_close(original);

    const auto id=nonce();
    const auto stage=data.parent_path()/(".ModeratedLMS_RestoreStage_"+id);
    const auto previousDb=data/("lms.sqlite3.pre_restore_"+id);
    const auto previousAttachments=data/("attachments.pre_restore_"+id);
    bool oldDbMoved=false,oldAttachmentsMoved=false,newDbMoved=false,newAttachmentsMoved=false;
    try {
        fs::create_directories(stage/"attachments"/"files");
        fs::copy_file(backup/"lms.sqlite3",stage/"lms.sqlite3");
        copyAttachments(backup/"attachments"/"files",stage/"attachments"/"files");
        // A second consistency check after copying prevents a truncated staging copy.
        const auto stagingPackage=stage/"manifest.txt";
        fs::copy_file(backup/"manifest.txt",stagingPackage);
        verify(stage);
        if(!fs::exists(data/"attachments"))
            fs::create_directories(data/"attachments"/"files");
        fs::rename(data/"lms.sqlite3",previousDb);
        oldDbMoved=true;
        fs::rename(data/"attachments",previousAttachments);
        oldAttachmentsMoved=true;
        fs::rename(stage/"lms.sqlite3",data/"lms.sqlite3");
        newDbMoved=true;
        fs::rename(stage/"attachments",data/"attachments");
        newAttachmentsMoved=true;
        // Roll forward only after both new components reached their destinations.
        // Finalisation is complete. Cleanup errors must never roll back an
        // already-completed restore after the old file has been removed.
        std::error_code cleanup;
        fs::remove(previousDb,cleanup);
        cleanup.clear();
        fs::remove_all(previousAttachments,cleanup);
        cleanup.clear();
        fs::remove_all(stage,cleanup);
        return safety;
    } catch(...) {
        std::error_code ec;
        if(newDbMoved)fs::remove(data/"lms.sqlite3",ec);
        if(newAttachmentsMoved)fs::remove_all(data/"attachments",ec);
        if(oldDbMoved)fs::rename(previousDb,data/"lms.sqlite3",ec);
        if(oldAttachmentsMoved)fs::rename(previousAttachments,data/"attachments",ec);
        fs::remove_all(stage,ec);
        throw;
    }
}
} // namespace lms
