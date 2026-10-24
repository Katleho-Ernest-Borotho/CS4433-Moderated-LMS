#include "lms/application/AcademicManagementService.h"
#include "lms/application/AuthenticationService.h"
#include "lms/application/BootstrapService.h"
#include "lms/domain/User.h"
#include "lms/persistence/SQLiteRecordRepository.h"
#include "lms/persistence/SqliteDatabase.h"
#include "lms/security/SodiumPasswordHasher.h"

#include <gtest/gtest.h>
#include <sqlite3.h>
#include <algorithm>
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

class AuthenticationTests : public ::testing::Test {
protected:
    void SetUp() override
    {
        database.initializeSchemaFromFile(
            (std::filesystem::current_path() / "lms_schema.sql").string());
    }

    lms::SqliteDatabase database{":memory:"};
    lms::SodiumPasswordHasher passwords;
    lms::SQLiteRecordRepository users{database};
    lms::AuthenticationService authentication{users, passwords};
    lms::BootstrapService bootstrap{users, passwords};
};

TEST_F(AuthenticationTests, FreshDatabaseHasNoDefaultCredentials)
{
    EXPECT_TRUE(bootstrap.needsSetup());
    EXPECT_FALSE(users.findByUsername("admin"));
}

TEST_F(AuthenticationTests, CanCreateAndLogInAsFirstAdministrator)
{
    bootstrap.createInitialAdministrator("school.admin", "LongSecretPassword123!");
    ASSERT_FALSE(bootstrap.needsSetup());
    auto user = users.findByUsername("SCHOOL.ADMIN");
    ASSERT_NE(user, nullptr);
    EXPECT_EQ(user->role(), lms::Role::Administrator);
    EXPECT_NE(user->passwordHash(), "LongSecretPassword123!");
    EXPECT_TRUE(passwords.verify("LongSecretPassword123!", user->passwordHash()));

    const auto session = authentication.logIn("SCHOOL.ADMIN", "LongSecretPassword123!");
    EXPECT_EQ(session.userId(), user->id());
    EXPECT_EQ(session.role(), lms::Role::Administrator);
    authentication.requireRole(session, lms::Role::Administrator);
}

TEST_F(AuthenticationTests, RejectsIncorrectOrUnknownLogin)
{
    bootstrap.createInitialAdministrator("schooladmin", "LongSecretPassword123!");
    EXPECT_THROW(authentication.logIn("schooladmin", "wrong password"), lms::AuthenticationError);
    EXPECT_THROW(authentication.logIn("unknown", "LongSecretPassword123!"), lms::AuthenticationError);
    EXPECT_THROW(authentication.logIn("", "LongSecretPassword123!"), lms::AuthenticationError);
    EXPECT_THROW(authentication.logIn("schooladmin", ""), lms::AuthenticationError);
}

TEST_F(AuthenticationTests, RejectsSecondBootstrap)
{
    bootstrap.createInitialAdministrator("firstadmin", "LongSecretPassword123!");
    EXPECT_THROW(bootstrap.createInitialAdministrator("otheradmin", "LongSecretPassword123!"),
                 std::runtime_error);
    EXPECT_FALSE(users.findByUsername("otheradmin"));
}

TEST_F(AuthenticationTests, RejectsWeakSetupCredentials)
{
    EXPECT_THROW(bootstrap.createInitialAdministrator("a", "LongSecretPassword123!"),
                 std::invalid_argument);
    EXPECT_THROW(bootstrap.createInitialAdministrator("root", "short"),
                 std::invalid_argument);
    EXPECT_TRUE(bootstrap.needsSetup());
}

TEST_F(AuthenticationTests, InactiveAccountCannotLogInOrReuseSession)
{
    bootstrap.createInitialAdministrator("admin1", "LongSecretPassword123!");
    const auto session = authentication.logIn("admin1", "LongSecretPassword123!");
    // Keep a second active administrator; the repository protects the last one.
    users.saveUser("anotheradmin", passwords.hash("LongSecretPassword456!"),
                   lms::Role::Administrator);
    users.deactivateUser(session.userId());
    EXPECT_THROW(authentication.logIn("admin1", "LongSecretPassword123!"),
                 lms::AuthenticationError);
    EXPECT_THROW(authentication.requireRole(session, lms::Role::Administrator),
                 lms::AuthenticationError);
}

TEST_F(AuthenticationTests, ChecksRoleOnEveryProtectedServiceCall)
{
    bootstrap.createInitialAdministrator("admin1", "LongSecretPassword123!");
    const auto session = authentication.logIn("admin1", "LongSecretPassword123!");
    EXPECT_NO_THROW(authentication.requireRole(session, lms::Role::Administrator));
    EXPECT_THROW(authentication.requireRole(session, lms::Role::Student),
                 lms::AuthenticationError);
}

TEST_F(AuthenticationTests, PreparedUsernameQueryDoesNotPermitSqlInjection)
{
    bootstrap.createInitialAdministrator("admin1", "LongSecretPassword123!");
    EXPECT_FALSE(users.findByUsername("' OR 1=1 --"));
    EXPECT_THROW(authentication.logIn("' OR 1=1 --", "LongSecretPassword123!"),
                 lms::AuthenticationError);
}

class UserManagementTests : public AuthenticationTests {
protected:
    lms::AcademicManagementService management{authentication, users, passwords};
};

TEST_F(UserManagementTests, AdministratorCanCreateAllFourRolesAndListAccounts)
{
    bootstrap.createInitialAdministrator("firstadmin", "LongSecretPassword123!");
    const auto admin = authentication.logIn("firstadmin", "LongSecretPassword123!");
    const auto student = management.createUser(admin, "student1", "StudentPassword123!", lms::Role::Student);
    const auto lecturer = management.createUser(admin, "lecturer1", "LecturerPassword123!", lms::Role::Lecturer);
    const auto hod = management.createUser(admin, "hod1", "DepartmentPass123!", lms::Role::HeadOfDepartment);
    const auto secondAdmin = management.createUser(admin, "admin2", "AnotherSecret123!", lms::Role::Administrator);
    EXPECT_EQ(student.role, lms::Role::Student);
    EXPECT_EQ(lecturer.role, lms::Role::Lecturer);
    EXPECT_EQ(hod.role, lms::Role::HeadOfDepartment);
    EXPECT_EQ(secondAdmin.role, lms::Role::Administrator);
    EXPECT_EQ(management.listUsers(admin).size(), 5U);
    EXPECT_EQ(authentication.logIn("student1", "StudentPassword123!").role(), lms::Role::Student);
    EXPECT_EQ(authentication.logIn("lecturer1", "LecturerPassword123!").role(), lms::Role::Lecturer);
    EXPECT_EQ(authentication.logIn("hod1", "DepartmentPass123!").role(), lms::Role::HeadOfDepartment);
}

TEST_F(UserManagementTests, NonAdministratorCannotManageAccounts)
{
    bootstrap.createInitialAdministrator("firstadmin", "LongSecretPassword123!");
    const auto admin = authentication.logIn("firstadmin", "LongSecretPassword123!");
    const auto student = management.createUser(admin, "student1", "StudentPassword123!", lms::Role::Student);
    const auto loggedInStudent = authentication.logIn("student1", "StudentPassword123!");
    EXPECT_THROW(management.listUsers(loggedInStudent), lms::AuthenticationError);
    EXPECT_THROW(management.createUser(loggedInStudent, "otherstudent",
        "AnotherSecret123!", lms::Role::Student), lms::AuthenticationError);
    EXPECT_THROW(management.deactivateUser(loggedInStudent, student.id), lms::AuthenticationError);
    EXPECT_EQ(management.listUsers(admin).size(), 2U);
}

TEST_F(UserManagementTests, DuplicateUsernamesAreRejectedRegardlessOfCase)
{
    bootstrap.createInitialAdministrator("firstadmin", "LongSecretPassword123!");
    const auto admin = authentication.logIn("firstadmin", "LongSecretPassword123!");
    management.createUser(admin, "student1", "StudentPassword123!", lms::Role::Student);
    EXPECT_THROW(management.createUser(admin, "STUDENT1",
        "DifferentSecret123!", lms::Role::Student), std::invalid_argument);
    EXPECT_EQ(management.listUsers(admin).size(), 2U);
}

TEST_F(UserManagementTests, DeactivatedStudentIsListedInactiveAndCannotLogin)
{
    bootstrap.createInitialAdministrator("firstadmin", "LongSecretPassword123!");
    const auto admin = authentication.logIn("firstadmin", "LongSecretPassword123!");
    const auto student = management.createUser(admin, "student1", "StudentPassword123!", lms::Role::Student);
    const auto loggedInStudent = authentication.logIn("student1", "StudentPassword123!");
    management.deactivateUser(admin, student.id);
    EXPECT_THROW(authentication.logIn("student1", "StudentPassword123!"), lms::AuthenticationError);
    EXPECT_THROW(authentication.requireRole(loggedInStudent, lms::Role::Student), lms::AuthenticationError);
    const auto accounts = management.listUsers(admin);
    ASSERT_EQ(accounts.size(), 2U);
    auto found = std::find_if(accounts.begin(), accounts.end(),
        [&student](const lms::UserAccount& u) { return u.id == student.id; });
    ASSERT_NE(found, accounts.end());
    EXPECT_FALSE(found->active);
}

TEST_F(UserManagementTests, CannotDeactivateSelfOrLastActiveAdministrator)
{
    bootstrap.createInitialAdministrator("firstadmin", "LongSecretPassword123!");
    const auto admin = authentication.logIn("firstadmin", "LongSecretPassword123!");
    EXPECT_THROW(management.deactivateUser(admin, admin.userId()), std::invalid_argument);
    EXPECT_THROW(users.deactivateUser(admin.userId()), std::invalid_argument);
    const auto secondAdmin = management.createUser(admin, "admin2",
        "AnotherSecret123!", lms::Role::Administrator);
    EXPECT_NO_THROW(management.deactivateUser(admin, secondAdmin.id));
    EXPECT_THROW(authentication.logIn("admin2", "AnotherSecret123!"), lms::AuthenticationError);
    EXPECT_THROW(users.deactivateUser(admin.userId()), std::invalid_argument);
}

TEST_F(UserManagementTests, RejectsInvalidNewAccountCredentials)
{
    bootstrap.createInitialAdministrator("firstadmin", "LongSecretPassword123!");
    const auto admin = authentication.logIn("firstadmin", "LongSecretPassword123!");
    EXPECT_THROW(management.createUser(admin, "x", "LongEnoughSecret123!", lms::Role::Student),
                 std::invalid_argument);
    EXPECT_THROW(management.createUser(admin, "student1", "short", lms::Role::Student),
                 std::invalid_argument);
    EXPECT_THROW(management.createUser(admin, "' OR 1=1 --", "LongEnoughSecret123!",
                                      lms::Role::Student), std::invalid_argument);
    EXPECT_EQ(management.listUsers(admin).size(), 1U);
}
