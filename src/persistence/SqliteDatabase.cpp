#include "lms/persistence/SqliteDatabase.h"

#include <fstream>
#include <sstream>
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

void SqliteDatabase::initializeSchemaFromFile(const std::string& schemaFile)
{
    std::ifstream input(schemaFile);
    if (!input) {
        throw std::runtime_error("Cannot read schema file: " + schemaFile);
    }
    std::ostringstream contents;
    contents << input.rdbuf();
    if (input.bad()) {
        throw std::runtime_error("Error reading schema file");
    }
    const std::string sql = contents.str();

    // Do not overwrite an existing database or silently accept a partial schema.
    sqlite3_stmt* statement = nullptr;
    const char* checkSql =
        "SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name IN ("
        "'users','departments','hod_departments','courses','enrollments',"
        "'teaching_assignments','assignments','deadline_extensions','submissions',"
        "'submission_versions','assessment_results','mark_change_audits');";
    if (sqlite3_prepare_v2(connection_, checkSql, -1, &statement, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Cannot inspect existing database");
    }
    const int status = sqlite3_step(statement);
    const int knownTables = status == SQLITE_ROW ? sqlite3_column_int(statement, 0) : -1;
    sqlite3_finalize(statement);
    if (knownTables < 0) {
        throw std::runtime_error("Cannot inspect existing database");
    }
    // v1.1 CR1 adds one table transactionally without altering the original 12.
    if (knownTables != 0 && knownTables != 12) {
        throw std::runtime_error("Database schema is incomplete; manual inspection is required");
    }

    char* message = nullptr;
    const auto execute = [this, &message](const char* statementSql) {
        const int rc = sqlite3_exec(connection_, statementSql, nullptr, nullptr, &message);
        if (rc != SQLITE_OK) {
            const std::string reason = message ? message : sqlite3_errmsg(connection_);
            sqlite3_free(message);
            message = nullptr;
            throw std::runtime_error(reason);
        }
    };

    try {
        execute("BEGIN IMMEDIATE;");
        if (knownTables == 0) execute(sql.c_str());
        execute(R"CR1(CREATE TABLE IF NOT EXISTS class_deadline_extensions (
 class_extension_id INTEGER PRIMARY KEY,
 assignment_id INTEGER NOT NULL UNIQUE REFERENCES assignments(assignment_id) ON DELETE RESTRICT,
 lecturer_id INTEGER NOT NULL REFERENCES users(user_id) ON DELETE RESTRICT,
 new_deadline_utc TEXT NOT NULL,
 created_at_utc TEXT NOT NULL
);
CREATE INDEX IF NOT EXISTS idx_class_extensions_lecturer ON class_deadline_extensions(lecturer_id);)CR1");
        execute("COMMIT;");
    } catch (...) {
        sqlite3_exec(connection_, "ROLLBACK;", nullptr, nullptr, nullptr);
        throw;
    }
}

} // namespace lms
