#include "lms/persistence/SQLiteRecordRepository.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

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

std::string roleToStorage(Role role)
{
    switch (role) {
    case Role::Student: return "Student";
    case Role::Lecturer: return "Lecturer";
    case Role::HeadOfDepartment: return "HeadOfDepartment";
    case Role::Administrator: return "Administrator";
    }
    throw std::invalid_argument("Invalid user role");
}

Role roleFromStorage(const std::string& value)
{
    if (value == "Student") return Role::Student;
    if (value == "Lecturer") return Role::Lecturer;
    if (value == "HeadOfDepartment") return Role::HeadOfDepartment;
    if (value == "Administrator") return Role::Administrator;
    throw std::runtime_error("Invalid role stored in database");
}

void bindText(sqlite3_stmt* statement, int index, std::string_view value)
{
    const std::string text(value);
    if (sqlite3_bind_text(statement, index, text.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
        throw std::runtime_error("Cannot bind database value");
    }
}

std::int64_t insertUser(sqlite3* db, std::string_view username,
                        std::string_view hash, Role role)
{
    auto statement = prepare(db,
        "INSERT INTO users(username,password_hash,role,active) VALUES(?1,?2,?3,1);");
    bindText(statement.get(), 1, username);
    bindText(statement.get(), 2, hash);
    bindText(statement.get(), 3, roleToStorage(role));
    if (sqlite3_step(statement.get()) != SQLITE_DONE) {
        if (sqlite3_extended_errcode(db) == SQLITE_CONSTRAINT_UNIQUE) {
            throw std::invalid_argument("Username already exists");
        }
        throw std::runtime_error(sqlite3_errmsg(db));
    }
    const std::int64_t id = sqlite3_last_insert_rowid(db);
    statement.reset();
    return id;
}

} // namespace

SQLiteRecordRepository::SQLiteRecordRepository(SqliteDatabase& database) : database_(database) {}

std::unique_ptr<User> SQLiteRecordRepository::findByUsername(std::string_view username) const
{
    sqlite3* db = database_.connection();
    auto statement = prepare(db,
        "SELECT user_id, username, password_hash, role, active "
        "FROM users WHERE username = ?1 COLLATE NOCASE LIMIT 1;");
    bindText(statement.get(), 1, username);

    const int result = sqlite3_step(statement.get());
    if (result == SQLITE_DONE) return nullptr;
    if (result != SQLITE_ROW) throw std::runtime_error(sqlite3_errmsg(db));

    const auto id = sqlite3_column_int64(statement.get(), 0);
    const std::string savedName = columnText(statement.get(), 1);
    const std::string hash = columnText(statement.get(), 2);
    const Role role = roleFromStorage(columnText(statement.get(), 3));
    const bool active = sqlite3_column_int(statement.get(), 4) == 1;
    switch (role) {
    case Role::Student: return std::make_unique<Student>(id, savedName, hash, active);
    case Role::Lecturer: return std::make_unique<Lecturer>(id, savedName, hash, active);
    case Role::HeadOfDepartment:
        return std::make_unique<HeadOfDepartment>(id, savedName, hash, active);
    case Role::Administrator:
        return std::make_unique<Administrator>(id, savedName, hash, active);
    }
    throw std::runtime_error("Unsupported user role");
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
        if (hasUsers()) throw std::runtime_error("Initial administrator already exists");
        insertUser(db, username, passwordHash, Role::Administrator);
        execute(db, "COMMIT;");
    } catch (...) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        throw;
    }
}

std::int64_t SQLiteRecordRepository::saveUser(std::string_view username,
                                              std::string_view passwordHash, Role role)
{
    sqlite3* db = database_.connection();
    execute(db, "BEGIN IMMEDIATE;");
    try {
        const auto id = insertUser(db, username, passwordHash, role);
        execute(db, "COMMIT;");
        return id;
    } catch (...) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        throw;
    }
}

std::vector<UserAccount> SQLiteRecordRepository::listUsers() const
{
    sqlite3* db = database_.connection();
    auto statement = prepare(db,
        "SELECT user_id, username, role, active FROM users ORDER BY username COLLATE NOCASE;");
    std::vector<UserAccount> result;
    for (;;) {
        const int step = sqlite3_step(statement.get());
        if (step == SQLITE_DONE) break;
        if (step != SQLITE_ROW) throw std::runtime_error(sqlite3_errmsg(db));
        result.push_back(UserAccount{
            sqlite3_column_int64(statement.get(), 0),
            columnText(statement.get(), 1),
            roleFromStorage(columnText(statement.get(), 2)),
            sqlite3_column_int(statement.get(), 3) == 1
        });
    }
    return result;
}

void SQLiteRecordRepository::deactivateUser(std::int64_t id)
{
    sqlite3* db = database_.connection();
    execute(db, "BEGIN IMMEDIATE;");
    try {
        auto row = prepare(db, "SELECT role, active FROM users WHERE user_id=?1;");
        if (sqlite3_bind_int64(row.get(), 1, id) != SQLITE_OK) {
            throw std::runtime_error("Cannot bind user identifier");
        }
        if (sqlite3_step(row.get()) != SQLITE_ROW) {
            throw std::invalid_argument("User not found");
        }
        const Role role = roleFromStorage(columnText(row.get(), 0));
        const bool active = sqlite3_column_int(row.get(), 1) == 1;
        row.reset();
        if (!active) throw std::invalid_argument("Account is already inactive");
        if (role == Role::Administrator) {
            auto count = prepare(db,
                "SELECT COUNT(*) FROM users WHERE role='Administrator' AND active=1;");
            if (sqlite3_step(count.get()) != SQLITE_ROW) {
                throw std::runtime_error(sqlite3_errmsg(db));
            }
            const int admins = sqlite3_column_int(count.get(), 0);
            count.reset();
            if (admins <= 1) {
                throw std::invalid_argument("Cannot deactivate the last active administrator");
            }
        }
        auto change = prepare(db, "UPDATE users SET active=0 WHERE user_id=?1 AND active=1;");
        if (sqlite3_bind_int64(change.get(), 1, id) != SQLITE_OK ||
            sqlite3_step(change.get()) != SQLITE_DONE) {
            throw std::runtime_error(sqlite3_errmsg(db));
        }
        if (sqlite3_changes(db) != 1) throw std::runtime_error("User update failed");
        change.reset();
        execute(db, "COMMIT;");
    } catch (...) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        throw;
    }
}

} // namespace lms
