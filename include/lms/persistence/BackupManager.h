#pragma once
#include <sqlite3.h>
#include <cstdint>
#include <filesystem>
#include <string>

namespace lms {
struct BackupInfo {
    std::filesystem::path folder;
    std::uint64_t attachmentCount=0;
    std::uint64_t totalBytes=0;
};
class BackupManager final {
public:
    // Creates a complete, verified folder under destinationParent. No live data is overwritten.
    static BackupInfo create(sqlite3* liveDatabase, const std::filesystem::path& dataRoot,
                             const std::filesystem::path& destinationParent);
    // Checks manifest, SHA-256 hashes, 12-table schema and referenced attachments.
    static BackupInfo verify(const std::filesystem::path& backupFolder);
    // OFFLINE ONLY. Creates a safety backup and replaces live data after validation.
    // The caller must own the same exclusive QLockFile used by the normal LMS.
    static std::filesystem::path restore(const std::filesystem::path& backupFolder,
                                          const std::filesystem::path& dataRoot);
};
} // namespace lms
