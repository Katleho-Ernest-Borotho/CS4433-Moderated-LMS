#include "lms/persistence/SQLiteRecordRepository.h"

#include <memory>
#include <stdexcept>
#include <string>

namespace lms {
namespace {
using Statement = std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;

Statement prepare(sqlite3* db, const char* sql)
{
    sqlite3_stmt* raw = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &raw, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }
    return Statement(raw, &sqlite3_finalize);
}

void execute(sqlite3* db, const char* sql)
{
    char* error = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &error) != SQLITE_OK) {
        const std::string message = error ? error : sqlite3_errmsg(db);
        sqlite3_free(error);
        throw std::runtime_error(message);
    }
}

std::string columnText(sqlite3_stmt* statement, int column)
{
    const auto* text = sqlite3_column_text(statement, column);
    return text ? reinterpret_cast<const char*>(text) : "";
}
} // namespace

SQLiteRecordRepository::SQLiteRecordRepository(SqliteDatabase& database) : database_(database) {}

std::unique_ptr<User> SQLiteRecordRepository::findByUsername(std::string_view username) const
{
    sqlite3* db = database_.connection();
    auto statement = prepare(db,
        "SELECT user_id, username, password_hash, role, active "
        "FROM users WHERE username = ?1 COLLATE NOCASE LIMIT 1;");
    const std::string name(username);
    if (sqlite3_bind_text(statement.get(), 1, name.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
        throw std::runtime_error("Could not bind username");
    }
    const int result = sqlite3_step(statement.get());
    if (result == SQLITE_DONE) return nullptr;
    if (result != SQLITE_ROW) throw std::runtime_error(sqlite3_errmsg(db));

    const auto id = sqlite3_column_int64(statement.get(), 0);
    const std::string savedName = columnText(statement.get(), 1);
    const std::string hash = columnText(statement.get(), 2);
    const std::string role = columnText(statement.get(), 3);
    const bool active = sqlite3_column_int(statement.get(), 4) == 1;

    if (role == "Student") return std::make_unique<Student>(id, savedName, hash, active);
    if (role == "Lecturer") return std::make_unique<Lecturer>(id, savedName, hash, active);
    if (role == "HeadOfDepartment") return std::make_unique<HeadOfDepartment>(id, savedName, hash, active);
    if (role == "Administrator") return std::make_unique<Administrator>(id, savedName, hash, active);
    throw std::runtime_error("Invalid role in users table");
}

bool SQLiteRecordRepository::hasUsers() const
{
    sqlite3* db = database_.connection();
    auto statement = prepare(db, "SELECT EXISTS(SELECT 1 FROM users LIMIT 1);");
    if (sqlite3_step(statement.get()) != SQLITE_ROW) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }
    return sqlite3_column_int(statement.get(), 0) == 1;
}

void SQLiteRecordRepository::bootstrapAdministrator(std::string_view username,
                                                     std::string_view passwordHash)
{
    sqlite3* db = database_.connection();
    execute(db, "BEGIN IMMEDIATE;");
    try {
        // A transaction prevents two running copies from bootstrapping different admins.
        if (hasUsers()) throw std::runtime_error("Initial administrator already exists");
        auto statement = prepare(db,
            "INSERT INTO users(username,password_hash,role,active) "
            "VALUES(?1,?2,'Administrator',1);");
        const std::string name(username), hash(passwordHash);
        if (sqlite3_bind_text(statement.get(), 1, name.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK ||
            sqlite3_bind_text(statement.get(), 2, hash.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
            throw std::runtime_error("Could not bind initial account");
        }
        if (sqlite3_step(statement.get()) != SQLITE_DONE) {
            throw std::runtime_error(sqlite3_errmsg(db));
        }
        statement.reset();
        execute(db, "COMMIT;");
    } catch (...) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        throw;
    }
}

void SQLiteRecordRepository::deactivateUser(std::int64_t id)
{
    sqlite3* db = database_.connection();
    auto statement = prepare(db, "UPDATE users SET active=0 WHERE user_id=?1;");
    if (sqlite3_bind_int64(statement.get(), 1, id) != SQLITE_OK ||
        sqlite3_step(statement.get()) != SQLITE_DONE) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }
    if (sqlite3_changes(db) != 1) throw std::runtime_error("User not found");
}

} // namespace lms
