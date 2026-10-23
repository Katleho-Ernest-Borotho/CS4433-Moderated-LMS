#include "lms/persistence/SqliteDatabase.h"

#include <stdexcept>
#include <string>

namespace lms {

SqliteDatabase::SqliteDatabase(const std::string& filePath)
{
    if (sqlite3_open_v2(filePath.c_str(), &connection_,
                        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK) {
        const std::string reason = connection_ ? sqlite3_errmsg(connection_) : "Unknown SQLite error";
        if (connection_) {
            sqlite3_close(connection_);
            connection_ = nullptr;
        }
        throw std::runtime_error("Cannot open database: " + reason);
    }
    char* errorMessage = nullptr;
    if (sqlite3_exec(connection_, "PRAGMA foreign_keys = ON;", nullptr, nullptr, &errorMessage) != SQLITE_OK) {
        const std::string reason = errorMessage ? errorMessage : "Cannot enable foreign keys";
        sqlite3_free(errorMessage);
        sqlite3_close(connection_);
        connection_ = nullptr;
        throw std::runtime_error(reason);
    }
}

SqliteDatabase::~SqliteDatabase()
{
    if (connection_) sqlite3_close(connection_);
}

bool SqliteDatabase::foreignKeysEnabled() const
{
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(connection_, "PRAGMA foreign_keys;", -1, &statement, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Cannot read foreign-key setting");
    }
    const bool enabled = sqlite3_step(statement) == SQLITE_ROW
                         && sqlite3_column_int(statement, 0) == 1;
    sqlite3_finalize(statement);
    return enabled;
}

} // namespace lms
