#include "lms/domain/User.h"
#include "lms/persistence/SqliteDatabase.h"
#include "lms/security/SodiumPasswordHasher.h"

#include <gtest/gtest.h>
#include <sqlite3.h>
#include <chrono>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>

TEST(UserTests, SupportsFourRolesThroughAbstractUser)
{
    std::unique_ptr<lms::User> users[] = {
        std::make_unique<lms::Student>(1, "student", "test-hash"),
        std::make_unique<lms::Lecturer>(2, "lecturer", "test-hash"),
        std::make_unique<lms::HeadOfDepartment>(3, "hod", "test-hash"),
        std::make_unique<lms::Administrator>(4, "admin", "test-hash")
    };
    EXPECT_EQ(users[0]->role(), lms::Role::Student);
    EXPECT_EQ(users[1]->role(), lms::Role::Lecturer);
    EXPECT_EQ(users[2]->role(), lms::Role::HeadOfDepartment);
    EXPECT_EQ(users[3]->role(), lms::Role::Administrator);
}

TEST(UserTests, RejectsMissingUsername)
{
    EXPECT_THROW(lms::Student(1, "", "test-hash"), std::invalid_argument);
}

TEST(PasswordTests, SaltedHashesVerifyWithoutStoringPlaintext)
{
    lms::SodiumPasswordHasher hasher;
    const std::string first = hasher.hash("test-password-123");
    const std::string second = hasher.hash("test-password-123");
    EXPECT_NE(first, "test-password-123");
    EXPECT_NE(first, second);
    EXPECT_TRUE(hasher.verify("test-password-123", first));
    EXPECT_FALSE(hasher.verify("wrong-password", first));
}

TEST(DatabaseTests, EnablesForeignKeysForEachConnection)
{
    lms::SqliteDatabase database(":memory:");
    EXPECT_TRUE(database.foreignKeysEnabled());
}

TEST(DatabaseTests, CreatesApprovedSchemaAndKeepsDataAfterReopen)
{
    namespace fs = std::filesystem;
    const std::string unique = std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    const fs::path databasePath = fs::temp_directory_path() / ("lms_schema_test_" + unique + ".sqlite3");
    const std::string schemaPath = (fs::current_path() / "lms_schema.sql").string();

    {
        lms::SqliteDatabase database(databasePath.string());
        database.initializeSchemaFromFile(schemaPath);
    }

    sqlite3* connection = nullptr;
    ASSERT_EQ(sqlite3_open(databasePath.string().c_str(), &connection), SQLITE_OK);
    sqlite3_stmt* statement = nullptr;
    ASSERT_EQ(sqlite3_prepare_v2(connection,
        "SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%';",
        -1, &statement, nullptr), SQLITE_OK);
    ASSERT_EQ(sqlite3_step(statement), SQLITE_ROW);
    EXPECT_EQ(sqlite3_column_int(statement, 0), 12);
    sqlite3_finalize(statement);
    ASSERT_EQ(sqlite3_exec(connection,
        "INSERT INTO users(username, password_hash, role) VALUES('test-user','dummy-hash','Student');",
        nullptr, nullptr, nullptr), SQLITE_OK);
    sqlite3_close(connection);

    {
        lms::SqliteDatabase database(databasePath.string());
        database.initializeSchemaFromFile(schemaPath);
    }

    ASSERT_EQ(sqlite3_open(databasePath.string().c_str(), &connection), SQLITE_OK);
    ASSERT_EQ(sqlite3_prepare_v2(connection, "SELECT COUNT(*) FROM users;",
        -1, &statement, nullptr), SQLITE_OK);
    ASSERT_EQ(sqlite3_step(statement), SQLITE_ROW);
    EXPECT_EQ(sqlite3_column_int(statement, 0), 1);
    sqlite3_finalize(statement);
    sqlite3_close(connection);
    fs::remove(databasePath);
}
