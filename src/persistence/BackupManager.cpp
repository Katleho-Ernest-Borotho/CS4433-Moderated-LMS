#include "lms/persistence/BackupManager.h"
#include <sodium.h>
#include <sqlite3.h>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>

namespace lms {
namespace {
namespace fs=std::filesystem;
using Statement=std::unique_ptr<sqlite3_stmt,decltype(&sqlite3_finalize)>;
Statement prepare(sqlite3* db,const char* q) {
    sqlite3_stmt* raw=nullptr;
    if(sqlite3_prepare_v2(db,q,-1,&raw,nullptr)!=SQLITE_OK)
        throw std::runtime_error(sqlite3_errmsg(db));
    return Statement(raw,&sqlite3_finalize);
}
void file(const fs::path& p) {
    if(fs::symlink_status(p).type()!=fs::file_type::regular)
        throw std::runtime_error("Missing or unsafe file: "+p.string());
}
void directory(const fs::path& p) {
    if(fs::symlink_status(p).type()!=fs::file_type::directory)
        throw std::runtime_error("Missing or unsafe directory: "+p.string());
}
bool valid(const std::string& s) {
    if(s.size()!=36 && s.size()!=37)return false;
    if(s[32]!='.')return false;
    const auto ext=s.substr(32);
    if(ext!=".pdf" && ext!=".docx" && ext!=".txt")return false;
    for(int i=0;i<32;++i) {
        const char c=s[static_cast<std::size_t>(i)];
        if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))return false;
    }
    return true;
}
std::string sha(const fs::path& p) {
    file(p);
    std::ifstream in(p,std::ios::binary);
    if(!in)throw std::runtime_error("Cannot read backup file");
    crypto_hash_sha256_state state{};
    crypto_hash_sha256_init(&state);
    std::array<char,65536> block{};
    while(in) {
        in.read(block.data(),block.size());
        const auto n=in.gcount();
        if(n>0)crypto_hash_sha256_update(&state,
            reinterpret_cast<const unsigned char*>(block.data()),
            static_cast<unsigned long long>(n));
    }
    if(!in.eof())throw std::runtime_error("Backup read failed");
    std::array<unsigned char,crypto_hash_sha256_BYTES> hash{};
    crypto_hash_sha256_final(&state,hash.data());
    std::array<char,crypto_hash_sha256_BYTES*2+1> text{};
    sodium_bin2hex(text.data(),text.size(),hash.data(),hash.size());
    return std::string(text.data());
}
void databaseCheck(const fs::path& p,const fs::path& attachments) {
    file(p);
    sqlite3* db=nullptr;
    if(sqlite3_open_v2(p.string().c_str(),&db,SQLITE_OPEN_READONLY,nullptr)!=SQLITE_OK) {
        if(db)sqlite3_close(db);
        throw std::runtime_error("Cannot open backup SQLite database");
    }
    try {
        auto integrity=prepare(db,"PRAGMA integrity_check;");
        if(sqlite3_step(integrity.get())!=SQLITE_ROW ||
           std::string(reinterpret_cast<const char*>(
                sqlite3_column_text(integrity.get(),0)))!="ok")
            throw std::runtime_error("SQLite integrity check failed");
        integrity.reset();
        auto tables=prepare(db,
            "SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name IN ("
            "'users','departments','hod_departments','courses','enrollments',"
            "'teaching_assignments','assignments','deadline_extensions',"
            "'submissions','submission_versions','assessment_results','mark_change_audits');");
        if(sqlite3_step(tables.get())!=SQLITE_ROW||
           sqlite3_column_int(tables.get(),0)!=12)
            throw std::runtime_error("Backup does not contain the approved 12 tables");
        tables.reset();
        auto foreignKeys=prepare(db,"PRAGMA foreign_key_check;");
        if(sqlite3_step(foreignKeys.get())!=SQLITE_DONE)
            throw std::runtime_error("Backup has invalid foreign key references");
        foreignKeys.reset();
        auto refs=prepare(db,"SELECT DISTINCT attachment_ref FROM submission_versions "
                             "WHERE attachment_ref IS NOT NULL AND attachment_ref!='';");
        while(true) {
            const auto rc=sqlite3_step(refs.get());
            if(rc==SQLITE_DONE)break;
            if(rc!=SQLITE_ROW)throw std::runtime_error(sqlite3_errmsg(db));
            const std::string name=reinterpret_cast<const char*>(
                sqlite3_column_text(refs.get(),0));
            if(!valid(name))throw std::runtime_error("Bad attachment reference");
            file(attachments/name);
        }
    } catch(...) {sqlite3_close(db);throw;}
    sqlite3_close(db);
}
void onlineBackup(sqlite3* src,const fs::path& path) {
    sqlite3* dst=nullptr;
    if(sqlite3_open_v2(path.string().c_str(),&dst,
        SQLITE_OPEN_CREATE|SQLITE_OPEN_READWRITE,nullptr)!=SQLITE_OK) {
        if(dst)sqlite3_close(dst);
        throw std::runtime_error("Could not create backup database");
    }
    auto* task=sqlite3_backup_init(dst,"main",src,"main");
    if(!task) {
        sqlite3_close(dst);
        throw std::runtime_error("SQLite backup initialization failed");
    }
    const int step=sqlite3_backup_step(task,-1);
    const int complete=sqlite3_backup_finish(task);
    sqlite3_close(dst);
    if(step!=SQLITE_DONE||complete!=SQLITE_OK)
        throw std::runtime_error("SQLite backup copy failed");
}
std::string nonce() {
    std::array<unsigned char,8> raw{};
    randombytes_buf(raw.data(),raw.size());
    std::array<char,17> hex{};
    sodium_bin2hex(hex.data(),hex.size(),raw.data(),raw.size());
    return std::string(hex.data());
}
void line(std::ofstream& out,const std::string& type,
          const std::string& name,const fs::path& p) {
    out<<type<<"\t"<<name<<"\t"<<fs::file_size(p)<<"\t"<<sha(p)<<"\n";
}
} // namespace

BackupInfo BackupManager::create(sqlite3* db,const fs::path& data,
                                 const fs::path& destination)
{
    if(sodium_init()<0)throw std::runtime_error("libsodium initialization failed");
    if(!db)throw std::invalid_argument("Missing database connection");
    directory(data);
    fs::create_directories(destination);
    directory(destination);
    // Never store backups inside the live application-data tree.
    {
        auto child=fs::weakly_canonical(destination);
        auto base=fs::weakly_canonical(data);
        auto i=child.begin();
        bool inside=true;
        for(auto parent=base.begin();parent!=base.end();++parent,++i) {
            if(i==child.end()||*i!=*parent){inside=false;break;}
        }
        if(inside)throw std::invalid_argument(
            "Choose a backup destination outside LMS application data");
    }
    auto name="ModeratedLMS_Backup_"+
        std::to_string(std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now()))+"_"+nonce();
    const auto work=destination/("."+name+".partial");
    const auto final=destination/name;
    fs::create_directories(work/"attachments"/"files");
    try {
        onlineBackup(db,work/"lms.sqlite3");
        const auto storage=data/"attachments"/"files";
        if(fs::exists(storage)) {
            directory(storage);
            for(const auto& item:fs::directory_iterator(storage)) {
                file(item.path());
                auto filename=item.path().filename().string();
                if(!valid(filename))throw std::runtime_error("Unexpected attachment filename");
                fs::copy_file(item.path(),work/"attachments"/"files"/filename);
            }
        }
        databaseCheck(work/"lms.sqlite3",work/"attachments"/"files");
        std::ofstream out(work/"manifest.txt",std::ios::binary);
        if(!out)throw std::runtime_error("Cannot write backup manifest");
        out<<"MODERATED_LMS_BACKUP_V1\n";
        line(out,"DB","lms.sqlite3",work/"lms.sqlite3");
        for(const auto& item:fs::directory_iterator(work/"attachments"/"files"))
            line(out,"FILE",item.path().filename().string(),item.path());
        out.close();
        if(!out)throw std::runtime_error("Backup manifest was not saved");
        fs::rename(work,final);
        return verify(final);
    } catch(...) {
        std::error_code ec;
        fs::remove_all(work,ec);
        throw;
    }
}

BackupInfo BackupManager::verify(const fs::path& backup)
{
    if(sodium_init()<0)throw std::runtime_error("libsodium initialization failed");
    directory(backup);
    directory(backup/"attachments"/"files");
    file(backup/"manifest.txt");
    std::ifstream in(backup/"manifest.txt",std::ios::binary);
    std::string first;
    if(!std::getline(in,first)||first!="MODERATED_LMS_BACKUP_V1")
        throw std::runtime_error("Not a compatible LMS backup");
    bool dbFound=false;
    BackupInfo info{backup,0,0};
    std::set<std::string> filenames;
    std::string row;
    while(std::getline(in,row)) {
        std::istringstream lineStream(row);
        std::string kind,name,sizeText,checksum;
        if(!std::getline(lineStream,kind,'\t')||
           !std::getline(lineStream,name,'\t')||
           !std::getline(lineStream,sizeText,'\t')||
           !std::getline(lineStream,checksum,'\t'))
            throw std::runtime_error("Damaged backup manifest");
        if(sizeText.empty() || sizeText.find_first_not_of("0123456789")!=std::string::npos)
            throw std::runtime_error("Bad size in backup manifest");
        const auto size=std::stoull(sizeText);
        fs::path p;
        if(kind=="DB"&&name=="lms.sqlite3"&&!dbFound) {
            dbFound=true;
            p=backup/name;
        } else if(kind=="FILE"&&valid(name)&&filenames.insert(name).second) {
            p=backup/"attachments"/"files"/name;
            ++info.attachmentCount;
        } else throw std::runtime_error("Bad backup manifest entry");
        file(p);
        if(fs::file_size(p)!=size||sha(p)!=checksum)
            throw std::runtime_error("Backup integrity check failed for "+name);
        info.totalBytes+=size;
    }
    if(!dbFound)throw std::runtime_error("Backup database missing from manifest");
    for(const auto& item:fs::directory_iterator(backup/"attachments"/"files")) {
        file(item.path());
        if(!filenames.count(item.path().filename().string()))
            throw std::runtime_error("Unlisted attachment found in backup");
    }
    databaseCheck(backup/"lms.sqlite3",backup/"attachments"/"files");
    return info;
}
} // namespace lms
