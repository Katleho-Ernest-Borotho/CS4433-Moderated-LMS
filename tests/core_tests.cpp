#include "lms/application/AcademicManagementService.h"
#include "lms/application/AuthenticationService.h"
#include "lms/application/BootstrapService.h"
#include "lms/application/SubmissionManagementService.h"
#include "lms/application/MarkingService.h"
#include "lms/application/ModerationService.h"
#include "lms/domain/LatePenaltyPolicy.h"
#include "lms/domain/UtcDateTime.h"
#include "lms/domain/User.h"
#include "lms/persistence/SQLiteRecordRepository.h"
#include "lms/persistence/LocalSubmissionContentStore.h"
#include "lms/persistence/SqliteDatabase.h"
#include "lms/security/SodiumPasswordHasher.h"

#include <gtest/gtest.h>
#include <sqlite3.h>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <limits>
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
    lms::AcademicManagementService management{authentication, users, users, passwords};
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

TEST_F(UserManagementTests, AdministratorCreatesUpdatesAndListsDepartmentsAndCourses)
{
    bootstrap.createInitialAdministrator("admin", "LongSecretPassword123!");
    const auto actor = authentication.logIn("admin", "LongSecretPassword123!");
    auto department = management.createDepartment(actor, "Computing");
    EXPECT_EQ(department.name, "Computing");
    const auto course = management.createCourse(actor, department.id,
        "CS4433", "Software Engineering");
    ASSERT_EQ(management.listDepartments(actor).size(), 1U);
    ASSERT_EQ(management.listAllCourses(actor).size(), 1U);
    department.name = "Computer Science";
    management.updateDepartment(actor, department);
    EXPECT_EQ(management.listDepartments(actor).at(0).name, "Computer Science");
    auto updated = course;
    updated.title = "Software Engineering II";
    management.updateCourse(actor, updated);
    EXPECT_EQ(management.listAllCourses(actor).at(0).title, "Software Engineering II");
}

TEST_F(UserManagementTests, StudentSeesOnlyOwnEnrolledCourses)
{
    bootstrap.createInitialAdministrator("admin", "LongSecretPassword123!");
    const auto admin = authentication.logIn("admin", "LongSecretPassword123!");
    const auto department = management.createDepartment(admin, "Computing");
    const auto first = management.createCourse(admin, department.id, "CS4433", "Software Engineering");
    management.createCourse(admin, department.id, "CS4441", "Networks");
    management.createUser(admin, "student1", "StudentPassword123!", lms::Role::Student);
    management.createUser(admin, "student2", "StudentPassword456!", lms::Role::Student);
    const auto firstStudent = authentication.logIn("student1", "StudentPassword123!");
    const auto secondStudent = authentication.logIn("student2", "StudentPassword456!");
    EXPECT_TRUE(management.listVisibleCourses(firstStudent).empty());
    management.enrolStudent(admin, firstStudent.userId(), first.id);
    const auto visible = management.listVisibleCourses(firstStudent);
    ASSERT_EQ(visible.size(), 1U);
    EXPECT_EQ(visible.at(0).code, "CS4433");
    EXPECT_TRUE(management.listVisibleCourses(secondStudent).empty());
    EXPECT_EQ(management.listVisibleCourses(admin).size(), 2U);
}

TEST_F(UserManagementTests, LecturerCanOnlySeeAssignedCourses)
{
    bootstrap.createInitialAdministrator("admin", "LongSecretPassword123!");
    const auto admin = authentication.logIn("admin", "LongSecretPassword123!");
    const auto department = management.createDepartment(admin, "Computing");
    management.createCourse(admin, department.id, "CS4433", "Software Engineering");
    const auto second = management.createCourse(admin, department.id, "CS4441", "Networks");
    management.createUser(admin, "lecturer1", "LecturerPassword123!", lms::Role::Lecturer);
    const auto lecturer = authentication.logIn("lecturer1", "LecturerPassword123!");
    EXPECT_TRUE(management.listVisibleCourses(lecturer).empty());
    management.assignLecturer(admin, lecturer.userId(), second.id);
    const auto visible = management.listVisibleCourses(lecturer);
    ASSERT_EQ(visible.size(), 1U);
    EXPECT_EQ(visible.at(0).code, "CS4441");
}

TEST_F(UserManagementTests, HoDCanOnlySeeOwnDepartmentCourses)
{
    bootstrap.createInitialAdministrator("admin", "LongSecretPassword123!");
    const auto admin = authentication.logIn("admin", "LongSecretPassword123!");
    const auto computing = management.createDepartment(admin, "Computing");
    const auto electrical = management.createDepartment(admin, "Electrical Engineering");
    management.createCourse(admin, computing.id, "CS4433", "Software Engineering");
    management.createCourse(admin, electrical.id, "EE4401", "Digital Circuit Design");
    management.createUser(admin, "hod1", "DepartmentPass123!", lms::Role::HeadOfDepartment);
    const auto hod = authentication.logIn("hod1", "DepartmentPass123!");
    management.assignHoD(admin, hod.userId(), computing.id);
    const auto visible = management.listVisibleCourses(hod);
    ASSERT_EQ(visible.size(), 1U);
    EXPECT_EQ(visible.at(0).code, "CS4433");
}

TEST_F(UserManagementTests, StudentCannotUseAcademicAdministrationService)
{
    bootstrap.createInitialAdministrator("admin", "LongSecretPassword123!");
    const auto admin = authentication.logIn("admin", "LongSecretPassword123!");
    const auto dep = management.createDepartment(admin, "Computing");
    const auto course = management.createCourse(admin, dep.id, "CS4433", "Software Engineering");
    management.createUser(admin, "student1", "StudentPassword123!", lms::Role::Student);
    const auto student = authentication.logIn("student1", "StudentPassword123!");
    EXPECT_THROW(management.createDepartment(student, "Wrong"), lms::AuthenticationError);
    EXPECT_THROW(management.listDepartments(student), lms::AuthenticationError);
    EXPECT_THROW(management.listAllCourses(student), lms::AuthenticationError);
    EXPECT_THROW(management.createCourse(student, dep.id, "CS9999", "Unauthorized"), lms::AuthenticationError);
    EXPECT_THROW(management.enrolStudent(student, student.userId(), course.id), lms::AuthenticationError);
    EXPECT_THROW(management.assignLecturer(student, student.userId(), course.id), lms::AuthenticationError);
}

TEST_F(UserManagementTests, AssignmentsRequireCorrectActiveRoles)
{
    bootstrap.createInitialAdministrator("admin", "LongSecretPassword123!");
    const auto admin = authentication.logIn("admin", "LongSecretPassword123!");
    const auto dep = management.createDepartment(admin, "Computing");
    const auto course = management.createCourse(admin, dep.id, "CS4433", "Software Engineering");
    const auto student = management.createUser(admin, "student1", "StudentPassword123!", lms::Role::Student);
    EXPECT_THROW(management.assignLecturer(admin, student.id, course.id), std::invalid_argument);
    EXPECT_THROW(management.assignHoD(admin, student.id, dep.id), std::invalid_argument);
    EXPECT_NO_THROW(management.enrolStudent(admin, student.id, course.id));
    EXPECT_THROW(management.enrolStudent(admin, student.id, course.id), std::exception);
}

TEST_F(UserManagementTests, DuplicateCourseCodeAndDepartmentNamesAreRejected)
{
    bootstrap.createInitialAdministrator("admin", "LongSecretPassword123!");
    const auto actor = authentication.logIn("admin", "LongSecretPassword123!");
    const auto dep = management.createDepartment(actor, "Computing");
    EXPECT_THROW(management.createDepartment(actor, "computing"), std::exception);
    management.createCourse(actor, dep.id, "CS4433", "Software Engineering");
    EXPECT_THROW(management.createCourse(actor, dep.id, "cs4433", "Duplicate"), std::exception);
    EXPECT_EQ(management.listAllCourses(actor).size(), 1U);
}

TEST_F(UserManagementTests, CannotCreateCoursesUnderMissingDepartments)
{
    bootstrap.createInitialAdministrator("admin", "LongSecretPassword123!");
    const auto actor = authentication.logIn("admin", "LongSecretPassword123!");
    EXPECT_THROW(management.createCourse(actor, 999999, "CS4433", "Software Engineering"),
                 std::invalid_argument);
    EXPECT_THROW(management.createDepartment(actor, " "), std::invalid_argument);
    EXPECT_THROW(management.createCourse(actor, 0, "CS4433", "Software Engineering"),
                 std::invalid_argument);
}

TEST_F(UserManagementTests, DeactivatedAccountCannotReadVisibleCourses)
{
    bootstrap.createInitialAdministrator("admin", "LongSecretPassword123!");
    const auto admin = authentication.logIn("admin", "LongSecretPassword123!");
    const auto dep = management.createDepartment(admin, "Computing");
    const auto course = management.createCourse(admin, dep.id, "CS4433", "Software Engineering");
    const auto student = management.createUser(admin, "student1", "StudentPassword123!", lms::Role::Student);
    const auto loggedIn = authentication.logIn("student1", "StudentPassword123!");
    management.enrolStudent(admin, student.id, course.id);
    management.deactivateUser(admin, student.id);
    EXPECT_THROW(management.listVisibleCourses(loggedIn), lms::AuthenticationError);
}


class AssignmentAndExtensionTests : public UserManagementTests {
protected:
    lms::SubmissionManagementService submissions{authentication,users};

    void initialise()
    {
        bootstrap.createInitialAdministrator("admin1", "LongSecretPassword123!");
        const auto admin=authentication.logIn("admin1","LongSecretPassword123!");
        const auto dep=management.createDepartment(admin,"Computing");
        const auto course=management.createCourse(admin,dep.id,"CS4433","Software Engineering");
        management.createUser(admin,"lecturer1","LecturerPassword123!",lms::Role::Lecturer);
        management.createUser(admin,"lecturer2","LecturerPassword456!",lms::Role::Lecturer);
        management.createUser(admin,"student1","StudentPassword123!",lms::Role::Student);
        management.createUser(admin,"student2","StudentPassword456!",lms::Role::Student);
        const auto lecturer=authentication.logIn("lecturer1","LecturerPassword123!");
        const auto student=authentication.logIn("student1","StudentPassword123!");
        management.assignLecturer(admin,lecturer.userId(),course.id);
        management.enrolStudent(admin,student.userId(),course.id);
        courseId=course.id;
    }
    std::int64_t courseId=0;
    static constexpr const char* original="2030-10-28T12:00:00Z";
    static constexpr const char* extended="2030-10-30T12:00:00Z";
};

TEST_F(AssignmentAndExtensionTests, AssignedLecturerCanCreateAssignment)
{
    initialise();
    const auto lecturer=authentication.logIn("lecturer1","LecturerPassword123!");
    auto a=management.createAssignment(lecturer,courseId,"Lab 1",original,100.0);
    EXPECT_GT(a.id,0);
    EXPECT_EQ(a.courseId,courseId);
    EXPECT_EQ(a.deadlineUtc,original);
    const auto list=management.listCourseAssignments(lecturer,courseId);
    ASSERT_EQ(list.size(),1U);
    EXPECT_EQ(list[0].title,"Lab 1");
    EXPECT_EQ(list[0].maximumMark,100.0);
}

TEST_F(AssignmentAndExtensionTests, UnassignedLecturerCannotCreateOrInspectAssignments)
{
    initialise();
    const auto first=authentication.logIn("lecturer1","LecturerPassword123!");
    const auto other=authentication.logIn("lecturer2","LecturerPassword456!");
    management.createAssignment(first,courseId,"Lab 1",original,100.0);
    EXPECT_THROW(management.createAssignment(other,courseId,"Wrong",original,100.0),
                 lms::AuthenticationError);
    EXPECT_THROW(management.listCourseAssignments(other,courseId),
                 lms::AuthenticationError);
}

TEST_F(AssignmentAndExtensionTests, EnrolledStudentCanSeeAssignmentButOtherCannot)
{
    initialise();
    const auto lecturer=authentication.logIn("lecturer1","LecturerPassword123!");
    const auto own=authentication.logIn("student1","StudentPassword123!");
    const auto other=authentication.logIn("student2","StudentPassword456!");
    management.createAssignment(lecturer,courseId,"Lab 1",original,100.0);
    ASSERT_EQ(management.listCourseAssignments(own,courseId).size(),1U);
    EXPECT_THROW(management.listCourseAssignments(other,courseId),
                 lms::AuthenticationError);
    EXPECT_THROW(management.createAssignment(own,courseId,"Wrong",original,100.0),
                 lms::AuthenticationError);
}

TEST_F(AssignmentAndExtensionTests, LecturerGrantsIndividualExtensionOnlyToEnrolledStudent)
{
    initialise();
    const auto lecturer=authentication.logIn("lecturer1","LecturerPassword123!");
    const auto own=authentication.logIn("student1","StudentPassword123!");
    const auto other=authentication.logIn("student2","StudentPassword456!");
    const auto a=management.createAssignment(lecturer,courseId,"Lab 1",original,100.0);
    EXPECT_EQ(submissions.effectiveDeadline(own,a.id,own.userId()),original);
    submissions.grantExtension(lecturer,a.id,own.userId(),extended);
    EXPECT_EQ(submissions.effectiveDeadline(own,a.id,own.userId()),extended);
    EXPECT_THROW(submissions.effectiveDeadline(other,a.id,own.userId()),
                 lms::AuthenticationError);
    EXPECT_THROW(submissions.grantExtension(lecturer,a.id,other.userId(),extended),
                 std::invalid_argument);
}

TEST_F(AssignmentAndExtensionTests, CannotShortenAnExtensionAndCanExtendFurther)
{
    initialise();
    const auto lecturer=authentication.logIn("lecturer1","LecturerPassword123!");
    const auto own=authentication.logIn("student1","StudentPassword123!");
    const auto a=management.createAssignment(lecturer,courseId,"Lab 1",original,100.0);
    submissions.grantExtension(lecturer,a.id,own.userId(),extended);
    EXPECT_THROW(submissions.grantExtension(lecturer,a.id,own.userId(),original),
                 std::invalid_argument);
    EXPECT_THROW(submissions.grantExtension(lecturer,a.id,own.userId(),extended),
                 std::invalid_argument);
    submissions.grantExtension(lecturer,a.id,own.userId(),"2030-11-01T12:00:00Z");
    EXPECT_EQ(submissions.effectiveDeadline(own,a.id,own.userId()),
              "2030-11-01T12:00:00Z");
}

TEST_F(AssignmentAndExtensionTests, UnassignedLecturerAndStudentCannotGrantExtension)
{
    initialise();
    const auto lecturer=authentication.logIn("lecturer1","LecturerPassword123!");
    const auto other=authentication.logIn("lecturer2","LecturerPassword456!");
    const auto student=authentication.logIn("student1","StudentPassword123!");
    const auto a=management.createAssignment(lecturer,courseId,"Lab 1",original,100.0);
    EXPECT_THROW(submissions.grantExtension(other,a.id,student.userId(),extended),
                 lms::AuthenticationError);
    EXPECT_THROW(submissions.grantExtension(student,a.id,student.userId(),extended),
                 lms::AuthenticationError);
    EXPECT_EQ(submissions.effectiveDeadline(student,a.id,student.userId()),original);
}

TEST_F(AssignmentAndExtensionTests, AssignedLecturerCanListOnlyOwnEnrolledStudents)
{
    initialise();
    const auto lecturer=authentication.logIn("lecturer1","LecturerPassword123!");
    const auto other=authentication.logIn("lecturer2","LecturerPassword456!");
    const auto students=management.listEnrolledStudentsForCourse(lecturer,courseId);
    ASSERT_EQ(students.size(),1U);
    EXPECT_EQ(students[0].username,"student1");
    EXPECT_THROW(management.listEnrolledStudentsForCourse(other,courseId),
                 lms::AuthenticationError);
}

TEST_F(AssignmentAndExtensionTests, DatesAreStrictUTCAndValidateCalendar)
{
    EXPECT_NO_THROW(lms::validateUtcDateTime("2028-02-29T23:59:59Z"));
    EXPECT_THROW(lms::validateUtcDateTime("2030-02-29T12:00:00Z"),std::invalid_argument);
    EXPECT_THROW(lms::validateUtcDateTime("2030-13-01T12:00:00Z"),std::invalid_argument);
    EXPECT_THROW(lms::validateUtcDateTime("2030-10-28T25:00:00Z"),std::invalid_argument);
    EXPECT_THROW(lms::validateUtcDateTime("2030-10-28 12:00:00"),std::invalid_argument);
    EXPECT_THROW(lms::validateUtcDateTime("2030-10-28T12:00:00+02:00"),std::invalid_argument);
}

TEST_F(AssignmentAndExtensionTests, RejectsInvalidAssignmentAndExtensionInputs)
{
    initialise();
    const auto lecturer=authentication.logIn("lecturer1","LecturerPassword123!");
    const auto student=authentication.logIn("student1","StudentPassword123!");
    EXPECT_THROW(management.createAssignment(lecturer,courseId,"",original,100.0),
                 std::invalid_argument);
    EXPECT_THROW(management.createAssignment(lecturer,courseId,"Lab 1",original,0.0),
                 std::invalid_argument);
    EXPECT_THROW(management.createAssignment(lecturer,courseId,"Lab 1",original,
                   std::numeric_limits<double>::infinity()),std::invalid_argument);
    EXPECT_THROW(management.createAssignment(lecturer,courseId,"Lab 1",
                   "2030-02-29T12:00:00Z",100.0),std::invalid_argument);
    const auto a=management.createAssignment(lecturer,courseId,"Lab 1",original,100.0);
    EXPECT_THROW(submissions.grantExtension(lecturer,a.id,student.userId(),
                 "2030-10-30 12:00:00"),std::invalid_argument);
}

TEST_F(AssignmentAndExtensionTests, DeactivatedLecturerCannotCreateOrExtend)
{
    initialise();
    const auto admin=authentication.logIn("admin1","LongSecretPassword123!");
    const auto lecturer=authentication.logIn("lecturer1","LecturerPassword123!");
    const auto student=authentication.logIn("student1","StudentPassword123!");
    const auto a=management.createAssignment(lecturer,courseId,"Lab 1",original,100.0);
    management.deactivateUser(admin,lecturer.userId());
    EXPECT_THROW(management.createAssignment(lecturer,courseId,"Lab 2",original,100.0),
                 lms::AuthenticationError);
    EXPECT_THROW(submissions.grantExtension(lecturer,a.id,student.userId(),extended),
                 lms::AuthenticationError);
}

TEST_F(AssignmentAndExtensionTests, EffectiveDeadlineIsScopedToCorrectAssignment)
{
    initialise();
    const auto lecturer=authentication.logIn("lecturer1","LecturerPassword123!");
    const auto student=authentication.logIn("student1","StudentPassword123!");
    const auto a=management.createAssignment(lecturer,courseId,"Lab 1",original,100.0);
    const auto b=management.createAssignment(lecturer,courseId,"Lab 2","2030-10-31T12:00:00Z",50.0);
    submissions.grantExtension(lecturer,a.id,student.userId(),extended);
    EXPECT_EQ(submissions.effectiveDeadline(student,a.id,student.userId()),extended);
    EXPECT_EQ(submissions.effectiveDeadline(student,b.id,student.userId()),
              "2030-10-31T12:00:00Z");
}


class SubmissionTests : public AssignmentAndExtensionTests {
protected:
    std::filesystem::path testFilesDir = std::filesystem::temp_directory_path() /
        ("lms_submission_tests_" +
         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    lms::LocalSubmissionContentStore store{testFilesDir.string()};
    lms::SubmissionManagementService submissionService{authentication,users,users,store};
    void TearDown() override
    {
        std::error_code ignored;
        std::filesystem::remove_all(testFilesDir,ignored);
    }

    lms::Assignment makeAssignment(const char* deadline)
    {
        const auto lecturer=authentication.logIn("lecturer1","LecturerPassword123!");
        return management.createAssignment(lecturer,courseId,"Lab 1",deadline,100.0);
    }
    lms::Session studentSession()
    {
        return authentication.logIn("student1","StudentPassword123!");
    }
};

TEST_F(SubmissionTests, StudentCanSubmitTextAndReviewFirstVersion)
{
    initialise();
    const auto a=makeAssignment("2030-10-28T12:00:00Z");
    const auto first=submissionService.submit(studentSession(),a.id,{"First answer",""});
    EXPECT_EQ(first.versionNumber,1);
    EXPECT_FALSE(first.late);
    EXPECT_EQ(first.text,"First answer");
    EXPECT_TRUE(first.attachmentRef.empty());
    auto history=submissionService.listMyVersions(studentSession(),a.id);
    ASSERT_EQ(history.size(),1U);
    EXPECT_EQ(history.front().text,"First answer");
    auto row=users.findFor(studentSession().userId(),a.id);
    ASSERT_TRUE(row.has_value());
    EXPECT_EQ(row->state,"Submitted");
}

TEST_F(SubmissionTests, OnTimeResubmissionRetainsEarlierVersion)
{
    initialise();
    const auto a=makeAssignment("2030-10-28T12:00:00Z");
    const auto student=studentSession();
    const auto first=submissionService.submit(student,a.id,{"First draft",""});
    const auto second=submissionService.submit(student,a.id,{"Improved draft",""});
    EXPECT_EQ(second.submissionId,first.submissionId);
    EXPECT_EQ(second.versionNumber,2);
    EXPECT_NE(second.id,first.id);
    const auto history=submissionService.listMyVersions(student,a.id);
    ASSERT_EQ(history.size(),2U);
    EXPECT_EQ(history.at(0).text,"Improved draft");
    EXPECT_EQ(history.at(1).text,"First draft");
    EXPECT_EQ(history.at(1).versionNumber,1);
}

TEST_F(SubmissionTests, FirstTimeLateSubmissionIsAcceptedAndMarkedLate)
{
    initialise();
    const auto a=makeAssignment("2000-01-01T00:00:00Z");
    const auto saved=submissionService.submit(studentSession(),a.id,{"Late first attempt",""});
    EXPECT_TRUE(saved.late);
    EXPECT_EQ(saved.versionNumber,1);
    EXPECT_EQ(users.findFor(studentSession().userId(),a.id)->state,"Late");
}

TEST_F(SubmissionTests, LateReplacementIsRejectedWithoutChangingHistory)
{
    initialise();
    const auto a=makeAssignment("2000-01-01T00:00:00Z");
    const auto first=submissionService.submit(studentSession(),a.id,{"First late attempt",""});
    EXPECT_THROW(submissionService.submit(studentSession(),a.id,{"Not allowed",""}),
                 std::invalid_argument);
    const auto history=submissionService.listMyVersions(studentSession(),a.id);
    ASSERT_EQ(history.size(),1U);
    EXPECT_EQ(history.front().id,first.id);
    EXPECT_TRUE(history.front().late);
}

TEST_F(SubmissionTests, IndividualExtensionPermitsReplacementBeforeEffectiveDeadline)
{
    initialise();
    const auto a=makeAssignment("2000-01-01T00:00:00Z");
    const auto student=studentSession();
    const auto first=submissionService.submit(student,a.id,{"First late attempt",""});
    EXPECT_TRUE(first.late);
    const auto lecturer=authentication.logIn("lecturer1","LecturerPassword123!");
    submissions.grantExtension(lecturer,a.id,student.userId(),"2030-11-01T12:00:00Z");
    const auto second=submissionService.submit(student,a.id,{"Improved after extension",""});
    EXPECT_EQ(second.versionNumber,2);
    EXPECT_FALSE(second.late);
    const auto history=submissionService.listMyVersions(student,a.id);
    ASSERT_EQ(history.size(),2U);
    EXPECT_TRUE(history.at(1).late);
}

TEST_F(SubmissionTests, UnenrolledStudentCannotSubmitOrReadAnotherStudentsHistory)
{
    initialise();
    const auto a=makeAssignment("2030-10-28T12:00:00Z");
    submissionService.submit(studentSession(),a.id,{"Private answer",""});
    const auto outsider=authentication.logIn("student2","StudentPassword456!");
    EXPECT_THROW(submissionService.submit(outsider,a.id,{"Not enrolled",""}),
                 lms::AuthenticationError);
    EXPECT_THROW(submissionService.listMyVersions(outsider,a.id),
                 lms::AuthenticationError);
}

TEST_F(SubmissionTests, LecturerCannotSubmitAsStudent)
{
    initialise();
    const auto a=makeAssignment("2030-10-28T12:00:00Z");
    const auto lecturer=authentication.logIn("lecturer1","LecturerPassword123!");
    EXPECT_THROW(submissionService.submit(lecturer,a.id,{"Invalid",""}),
                 lms::AuthenticationError);
    EXPECT_THROW(submissionService.listMyVersions(lecturer,a.id),
                 lms::AuthenticationError);
}

TEST_F(SubmissionTests, RejectsEmptyAndWhitespaceOnlySubmission)
{
    initialise();
    const auto a=makeAssignment("2030-10-28T12:00:00Z");
    EXPECT_THROW(submissionService.submit(studentSession(),a.id,{"",""}),
                 std::invalid_argument);
    EXPECT_THROW(submissionService.submit(studentSession(),a.id,{"  \t\n",""}),
                 std::invalid_argument);
    EXPECT_FALSE(users.findFor(studentSession().userId(),a.id));
}

TEST_F(SubmissionTests, AttachmentIsCopiedAndCanBeRead)
{
    initialise();
    const auto a=makeAssignment("2030-10-28T12:00:00Z");
    const auto source=testFilesDir/"answer.txt";
    {
        std::ofstream file(source,std::ios::binary);
        file << "Sample file answer";
    }
    const auto saved=submissionService.submit(studentSession(),a.id,{"",source.string()});
    EXPECT_FALSE(saved.attachmentRef.empty());
    EXPECT_EQ(saved.versionNumber,1);
    EXPECT_EQ(store.read(lms::StoredRef{saved.attachmentRef}),"Sample file answer");
    const auto history=submissionService.listMyVersions(studentSession(),a.id);
    EXPECT_EQ(history.at(0).attachmentRef,saved.attachmentRef);
    EXPECT_TRUE(history.at(0).text.empty());
}

TEST_F(SubmissionTests, AttachmentPersistsAlongsidePreviousVersion)
{
    initialise();
    const auto a=makeAssignment("2030-10-28T12:00:00Z");
    const auto source=testFilesDir/"first.txt";
    {
        std::ofstream file(source,std::ios::binary);
        file << "Version one file";
    }
    const auto first=submissionService.submit(studentSession(),a.id,{"v1",source.string()});
    const auto second=submissionService.submit(studentSession(),a.id,{"v2",""});
    EXPECT_EQ(second.versionNumber,2);
    EXPECT_TRUE(second.attachmentRef.empty());
    EXPECT_EQ(store.read(lms::StoredRef{first.attachmentRef}),"Version one file");
}

TEST_F(SubmissionTests, RejectsUnsupportedAttachmentFormat)
{
    initialise();
    const auto a=makeAssignment("2030-10-28T12:00:00Z");
    const auto source=testFilesDir/"unsafe.exe";
    { std::ofstream file(source); file << "not a supported file"; }
    EXPECT_THROW(submissionService.submit(studentSession(),a.id,{"",source.string()}),
                 std::invalid_argument);
    EXPECT_FALSE(users.findFor(studentSession().userId(),a.id));
}

TEST_F(SubmissionTests, RejectsAttachmentLargerThanTenMegabytes)
{
    initialise();
    const auto a=makeAssignment("2030-10-28T12:00:00Z");
    const auto source=testFilesDir/"too-large.txt";
    {
        std::ofstream file(source,std::ios::binary);
        file.seekp(10*1024*1024);
        file.put('x');
    }
    EXPECT_THROW(submissionService.submit(studentSession(),a.id,{"",source.string()}),
                 std::invalid_argument);
    EXPECT_FALSE(users.findFor(studentSession().userId(),a.id));
}

TEST_F(SubmissionTests, RejectsOversizedSubmissionText)
{
    initialise();
    const auto a=makeAssignment("2030-10-28T12:00:00Z");
    EXPECT_THROW(submissionService.submit(studentSession(),a.id,
                 {std::string(50001,'x'),""}),std::invalid_argument);
    EXPECT_FALSE(users.findFor(studentSession().userId(),a.id));
}

TEST_F(SubmissionTests, RepoEnforcesDeadlineInsideTransaction)
{
    initialise();
    const auto a=makeAssignment("2000-01-01T00:00:00Z");
    const auto student=studentSession();
    const auto first=users.saveVersion(student.userId(),a.id,"first","",
                                      "1999-12-31T23:59:59Z");
    EXPECT_EQ(first.versionNumber,1);
    EXPECT_THROW(users.saveVersion(student.userId(),a.id,"late replacement","",
                 "2000-01-01T00:00:01Z"),std::invalid_argument);
    const auto history=users.listVersions(student.userId(),a.id);
    EXPECT_EQ(history.size(),1U);
}

TEST_F(SubmissionTests, RepositoryAcceptsReplacementExactlyAtEffectiveDeadline)
{
    initialise();
    const auto a=makeAssignment("2030-10-28T12:00:00Z");
    const auto student=studentSession();
    users.saveVersion(student.userId(),a.id,"draft","",
                      "2030-10-28T11:59:59Z");
    const auto second=users.saveVersion(student.userId(),a.id,"on time","",
                                       "2030-10-28T12:00:00Z");
    EXPECT_EQ(second.versionNumber,2);
    EXPECT_FALSE(second.late);
}

TEST_F(SubmissionTests, InvalidAttachmentRefIsNotReadOrDeleted)
{
    EXPECT_THROW(store.read(lms::StoredRef{"../../escape.txt"}),std::invalid_argument);
    EXPECT_NO_THROW(store.remove(lms::StoredRef{"../../escape.txt"}));
}


TEST(LatePenaltyPolicyTests, ExactBoundariesAndCommencedDays)
{
    const lms::CommencedDayPenaltyPolicy policy;
    const auto check=[&](const char* timestamp,int days,double penalty,double finalMark) {
        const auto actual=policy.calculate(75.0,100.0,timestamp,"2030-10-28T12:00:00Z");
        EXPECT_EQ(actual.commencedLateDays,days);
        EXPECT_DOUBLE_EQ(actual.penaltyMark,penalty);
        EXPECT_DOUBLE_EQ(actual.finalMark,finalMark);
    };
    check("2030-10-28T11:59:59Z",0,0,75);
    check("2030-10-28T12:00:00Z",0,0,75);
    check("2030-10-28T12:00:01Z",1,10,65);
    check("2030-10-29T12:00:00Z",1,10,65);
    check("2030-10-29T12:00:01Z",2,20,55);
    check("2030-10-30T12:00:00Z",2,20,55);
    check("2030-10-30T12:00:01Z",3,30,45);
    check("2030-10-31T12:00:00Z",3,30,45);
    check("2030-10-31T12:00:01Z",4,75,0);
}

TEST(LatePenaltyPolicyTests, LowEarnedMarksNeverBecomeNegative)
{
    const lms::CommencedDayPenaltyPolicy p;
    auto result=p.calculate(5.0,100.0,"2030-10-28T12:00:01Z","2030-10-28T12:00:00Z");
    EXPECT_DOUBLE_EQ(result.penaltyMark,10);
    EXPECT_DOUBLE_EQ(result.finalMark,0);
}

TEST(LatePenaltyPolicyTests, RejectsOutOfRangeAndNonFiniteMarks)
{
    const lms::CommencedDayPenaltyPolicy p;
    const auto mark=[&](double earned,double max) {
        return p.calculate(earned,max,"2030-10-28T12:00:01Z","2030-10-28T12:00:00Z");
    };
    EXPECT_THROW(mark(-1,100),std::invalid_argument);
    EXPECT_THROW(mark(101,100),std::invalid_argument);
    EXPECT_THROW(mark(50,0),std::invalid_argument);
    EXPECT_THROW(mark(std::numeric_limits<double>::infinity(),100),std::invalid_argument);
    EXPECT_THROW(mark(20,std::numeric_limits<double>::quiet_NaN()),std::invalid_argument);
}

TEST(LatePenaltyPolicyTests, HandlesCalendarAndLeapDayTransitions)
{
    const lms::CommencedDayPenaltyPolicy p;
    const auto same=p.calculate(80,100,"2028-03-01T00:00:00Z","2028-02-29T00:00:00Z");
    EXPECT_EQ(same.commencedLateDays,1);
    const auto later=p.calculate(80,100,"2028-03-01T00:00:01Z","2028-02-29T00:00:00Z");
    EXPECT_EQ(later.commencedLateDays,2);
}

class MarkingTests : public SubmissionTests {
protected:
    lms::CommencedDayPenaltyPolicy penalty;
    lms::MarkingService marking{authentication,users,users,penalty,store};
    lms::Session lecturer()
    {
        return authentication.logIn("lecturer1","LecturerPassword123!");
    }
};

TEST_F(MarkingTests, LecturerCanViewLatestSubmittedVersionAndSaveProvisionalMark)
{
    initialise();
    const auto assignment=makeAssignment("2030-10-28T12:00:00Z");
    const auto version=submissionService.submit(studentSession(),assignment.id,{"Review this answer",""});
    const auto rows=marking.listSubmissions(lecturer(),assignment.id);
    ASSERT_EQ(rows.size(),1U);
    EXPECT_EQ(rows[0].latestVersionId,version.id);
    EXPECT_EQ(rows[0].text,"Review this answer");
    const auto saved=marking.mark(lecturer(),rows[0].submissionId,75,"Good effort","");
    EXPECT_DOUBLE_EQ(saved.earnedMark,75);
    EXPECT_DOUBLE_EQ(saved.penaltyMark,0);
    EXPECT_DOUBLE_EQ(saved.finalMark,75);
    EXPECT_FALSE(saved.approved);
    EXPECT_TRUE(saved.releasedAtUtc.empty());
    EXPECT_EQ(saved.markedVersionId,version.id);
    EXPECT_EQ(users.findFor(studentSession().userId(),assignment.id)->state,"Marked");
    EXPECT_TRUE(marking.auditsForSubmission(lecturer(),rows[0].submissionId).empty());
}

TEST_F(MarkingTests, MarkingAppliesOneLateDayDeductionFromMaximum)
{
    initialise();
    const auto assignment=makeAssignment("2000-01-01T00:00:00Z");
    const auto student=studentSession();
    const auto version=users.saveVersion(student.userId(),assignment.id,"Late submission","",
        "2000-01-01T00:00:01Z");
    const auto actual=marking.mark(lecturer(),version.submissionId,75,"Marked late","");
    EXPECT_DOUBLE_EQ(actual.penaltyMark,10);
    EXPECT_DOUBLE_EQ(actual.finalMark,65);
}

TEST_F(MarkingTests, MoreThan72HoursLateForcesFinalMarkZero)
{
    initialise();
    const auto assignment=makeAssignment("2000-01-01T00:00:00Z");
    const auto version=submissionService.submit(studentSession(),assignment.id,{"Very late",""});
    const auto result=marking.mark(lecturer(),version.submissionId,80,"Late feedback","");
    EXPECT_EQ(result.finalMark,0);
    EXPECT_EQ(result.penaltyMark,80);
}

TEST_F(MarkingTests, WrongLecturerAndStudentCannotSeeOrMarkSubmissions)
{
    initialise();
    const auto assignment=makeAssignment("2030-10-28T12:00:00Z");
    const auto version=submissionService.submit(studentSession(),assignment.id,{"Private",""});
    const auto other=authentication.logIn("lecturer2","LecturerPassword456!");
    EXPECT_THROW(marking.listSubmissions(other,assignment.id),lms::AuthenticationError);
    EXPECT_THROW(marking.mark(other,version.submissionId,60,"No permission",""),
        lms::AuthenticationError);
    EXPECT_THROW(marking.listSubmissions(studentSession(),assignment.id),
        lms::AuthenticationError);
    EXPECT_THROW(marking.currentResult(studentSession(),version.submissionId),
        lms::AuthenticationError);
    EXPECT_THROW(marking.auditsForSubmission(studentSession(),version.submissionId),
        lms::AuthenticationError);
}

TEST_F(MarkingTests, RejectsInvalidMarksAndBlankFeedbackWithoutWritingResults)
{
    initialise();
    const auto assignment=makeAssignment("2030-10-28T12:00:00Z");
    const auto version=submissionService.submit(studentSession(),assignment.id,{"Answer",""});
    EXPECT_THROW(marking.mark(lecturer(),version.submissionId,-1,"Bad",""),std::invalid_argument);
    EXPECT_THROW(marking.mark(lecturer(),version.submissionId,101,"Bad",""),std::invalid_argument);
    EXPECT_THROW(marking.mark(lecturer(),version.submissionId,60," ",""),std::invalid_argument);
    EXPECT_FALSE(marking.currentResult(lecturer(),version.submissionId));
}

TEST_F(MarkingTests, ChangingEarnedAndFinalMarksCreatesAtomicAuditEntries)
{
    initialise();
    const auto assignment=makeAssignment("2030-10-28T12:00:00Z");
    const auto version=submissionService.submit(studentSession(),assignment.id,{"Answer",""});
    marking.mark(lecturer(),version.submissionId,60,"First assessment","");
    const auto updated=marking.mark(lecturer(),version.submissionId,80,
                                   "After review","Rechecked answer");
    EXPECT_DOUBLE_EQ(updated.earnedMark,80);
    EXPECT_DOUBLE_EQ(updated.finalMark,80);
    const auto audit=marking.auditsForSubmission(lecturer(),version.submissionId);
    ASSERT_EQ(audit.size(),2U);
    EXPECT_EQ(audit[0].field,"earned_mark");
    EXPECT_EQ(audit[1].field,"final_mark");
    EXPECT_DOUBLE_EQ(audit[0].oldValue,60);
    EXPECT_DOUBLE_EQ(audit[0].newValue,80);
    EXPECT_EQ(audit[0].reason,"Rechecked answer");
    EXPECT_EQ(users.findFor(studentSession().userId(),assignment.id)->state,"Marked");
}

TEST_F(MarkingTests, RejectsRevisionWithoutReasonAndPreservesPreviousResult)
{
    initialise();
    const auto assignment=makeAssignment("2030-10-28T12:00:00Z");
    const auto version=submissionService.submit(studentSession(),assignment.id,{"Answer",""});
    const auto original=marking.mark(lecturer(),version.submissionId,60,"Original","");
    EXPECT_THROW(marking.mark(lecturer(),version.submissionId,90,"Revised",""),
                 std::invalid_argument);
    const auto saved=marking.currentResult(lecturer(),version.submissionId);
    ASSERT_TRUE(saved.has_value());
    EXPECT_DOUBLE_EQ(saved->earnedMark,original.earnedMark);
    EXPECT_EQ(saved->feedback,"Original");
    EXPECT_TRUE(marking.auditsForSubmission(lecturer(),version.submissionId).empty());
}

TEST_F(MarkingTests, NewVersionNeedsRemarkAndPreservesEarlierVersion)
{
    initialise();
    const auto assignment=makeAssignment("2030-10-28T12:00:00Z");
    const auto first=submissionService.submit(studentSession(),assignment.id,{"First",""});
    const auto earlier=marking.mark(lecturer(),first.submissionId,60,"First mark","");
    const auto second=submissionService.submit(studentSession(),assignment.id,{"Second",""});
    ASSERT_EQ(users.listVersions(studentSession().userId(),assignment.id).size(),2U);
    EXPECT_EQ(marking.currentResult(lecturer(),first.submissionId)->markedVersionId,first.id);
    EXPECT_EQ(users.findFor(studentSession().userId(),assignment.id)->state,"Submitted");
    EXPECT_THROW(marking.mark(lecturer(),first.submissionId,75,"Updated",""),
                 std::invalid_argument);
    const auto newest=marking.mark(lecturer(),second.submissionId,75,
                                    "New version assessed","Student resubmitted");
    EXPECT_EQ(newest.markedVersionId,second.id);
    EXPECT_EQ(newest.resultId,earlier.resultId);
    EXPECT_EQ(users.findFor(studentSession().userId(),assignment.id)->state,"Marked");
    EXPECT_EQ(users.listVersions(studentSession().userId(),assignment.id).size(),2U);
}

TEST_F(MarkingTests, LecturerCanOpenOnlyAuthorisedLatestAttachment)
{
    initialise();
    const auto assignment=makeAssignment("2030-10-28T12:00:00Z");
    const auto file=testFilesDir/"submission.txt";
    {
        std::ofstream output(file,std::ios::binary);
        output<<"Mark this file";
    }
    const auto version=submissionService.submit(studentSession(),assignment.id,{"",file.string()});
    EXPECT_EQ(marking.readLatestAttachment(lecturer(),version.submissionId),
              "Mark this file");
    const auto stranger=authentication.logIn("lecturer2","LecturerPassword456!");
    EXPECT_THROW(marking.readLatestAttachment(stranger,version.submissionId),
                 lms::AuthenticationError);
    EXPECT_THROW(marking.readLatestAttachment(studentSession(),version.submissionId),
                 lms::AuthenticationError);
}

TEST_F(MarkingTests, LatePenaltyUsesIndividualExtension)
{
    initialise();
    const auto assignment=makeAssignment("2000-01-01T00:00:00Z");
    const auto student=studentSession();
    const auto version=users.saveVersion(student.userId(),assignment.id,"Answer","",
                                         "2000-01-02T00:00:01Z");
    submissions.grantExtension(lecturer(),assignment.id,student.userId(),
                               "2030-10-28T12:00:00Z");
    const auto marked=marking.mark(lecturer(),version.submissionId,75,
                                    "Extended deadline applies","");
    EXPECT_DOUBLE_EQ(marked.penaltyMark,0);
    EXPECT_DOUBLE_EQ(marked.finalMark,75);
}

TEST_F(MarkingTests, MissingSubmissionCannotBeMarked)
{
    initialise();
    EXPECT_THROW(marking.mark(lecturer(),999999,50,"No submission",""),
                 std::invalid_argument);
}


class ModerationTests : public MarkingTests {
protected:
    lms::ModerationService moderation{authentication,users,penalty,store};

    lms::Session hod()
    {
        return authentication.logIn("hod1","DepartmentPass123!");
    }
    std::int64_t makeDepartmentHod()
    {
        const auto admin=authentication.logIn("admin1","LongSecretPassword123!");
        const auto dept=management.listDepartments(admin).at(0);
        const auto account=management.createUser(admin,"hod1","DepartmentPass123!",
                                                 lms::Role::HeadOfDepartment);
        management.assignHoD(admin,account.id,dept.id);
        return account.id;
    }

    lms::SubmissionVersion markedPastDue()
    {
        initialise();
        makeDepartmentHod();
        const auto assignment=makeAssignment("2000-01-01T12:00:00Z");
        const auto student=studentSession();
        const auto version=users.saveVersion(student.userId(),assignment.id,
            "Well researched assignment","","2000-01-01T12:00:01Z");
        const auto assessment=marking.mark(lecturer(),version.submissionId,75,
                                           "Clear explanation","");
        EXPECT_FALSE(assessment.approved);
        return version;
    }
};

TEST_F(ModerationTests, HoDSeesOnlyAssignedDepartmentProvisionalResults)
{
    const auto submission=markedPastDue();
    const auto pending=moderation.listDepartmentResults(hod());
    ASSERT_EQ(pending.size(),1U);
    EXPECT_EQ(pending.at(0).submissionId,submission.submissionId);
    EXPECT_EQ(pending.at(0).submittedText,"Well researched assignment");
    EXPECT_FALSE(pending.at(0).approved);
    EXPECT_EQ(pending.at(0).earnedMark,75);
    EXPECT_EQ(pending.at(0).penaltyMark,10);
    EXPECT_EQ(pending.at(0).finalMark,65);
}

TEST_F(ModerationTests, ApprovalDoesNotReleasePrivateStudentMarks)
{
    const auto version=markedPastDue();
    EXPECT_TRUE(moderation.getReleasedResults(studentSession()).empty());
    moderation.moderate(hod(),version.submissionId,std::nullopt,"");
    EXPECT_TRUE(moderation.getReleasedResults(studentSession()).empty());
    const auto result=users.findResult(version.submissionId);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->approved);
    EXPECT_TRUE(result->releasedAtUtc.empty());
    const auto latest=users.findMarkableSubmission(version.submissionId);
    ASSERT_TRUE(latest.has_value());
    EXPECT_EQ(users.findFor(studentSession().userId(),latest->assignmentId)->state,
              "Moderated");
}

TEST_F(ModerationTests, HoDApprovalThenSeparateReleaseMakesMarkVisibleToOnlyOwnStudent)
{
    const auto version=markedPastDue();
    moderation.moderate(hod(),version.submissionId,std::nullopt,"");
    moderation.release(hod(),version.submissionId);
    const auto released=moderation.getReleasedResults(studentSession());
    ASSERT_EQ(released.size(),1U);
    EXPECT_DOUBLE_EQ(released[0].earnedMark,75);
    EXPECT_DOUBLE_EQ(released[0].penaltyMark,10);
    EXPECT_DOUBLE_EQ(released[0].finalMark,65);
    EXPECT_EQ(released[0].feedback,"Clear explanation");
    EXPECT_FALSE(released[0].releasedAtUtc.empty());
    EXPECT_TRUE(moderation.listDepartmentResults(hod()).empty());
    const auto other=authentication.logIn("student2","StudentPassword456!");
    EXPECT_TRUE(moderation.getReleasedResults(other).empty());
}

TEST_F(ModerationTests, ReleasingWithoutModerationIsRejected)
{
    const auto version=markedPastDue();
    EXPECT_THROW(moderation.release(hod(),version.submissionId),std::invalid_argument);
    EXPECT_TRUE(moderation.getReleasedResults(studentSession()).empty());
}

TEST_F(ModerationTests, SecondReleaseAndSecondApprovalAreRejected)
{
    const auto version=markedPastDue();
    moderation.moderate(hod(),version.submissionId,std::nullopt,"");
    EXPECT_THROW(moderation.moderate(hod(),version.submissionId,std::nullopt,""),
                 std::invalid_argument);
    moderation.release(hod(),version.submissionId);
    EXPECT_THROW(moderation.release(hod(),version.submissionId),lms::AuthenticationError);
    EXPECT_EQ(moderation.getReleasedResults(studentSession()).size(),1U);
}

TEST_F(ModerationTests, HoDAdjustingEarnedMarkGeneratesEarnedAndFinalAudit)
{
    const auto version=markedPastDue();
    moderation.moderate(hod(),version.submissionId,80.0,"Reviewed calculation");
    const auto saved=users.findResult(version.submissionId);
    ASSERT_TRUE(saved.has_value());
    EXPECT_DOUBLE_EQ(saved->earnedMark,80);
    EXPECT_DOUBLE_EQ(saved->penaltyMark,10);
    EXPECT_DOUBLE_EQ(saved->finalMark,70);
    EXPECT_TRUE(saved->approved);
    const auto audits=users.listMarkAudits(saved->resultId);
    ASSERT_EQ(audits.size(),2U);
    EXPECT_EQ(audits[0].field,"earned_mark");
    EXPECT_EQ(audits[1].field,"final_mark");
    EXPECT_DOUBLE_EQ(audits[0].oldValue,75);
    EXPECT_DOUBLE_EQ(audits[0].newValue,80);
    EXPECT_EQ(audits[0].actorId,hod().userId());
    EXPECT_EQ(audits[0].reason,"Reviewed calculation");
}

TEST_F(ModerationTests, AdjustedMarksRequireReasonAndRollbackUnapproved)
{
    const auto version=markedPastDue();
    EXPECT_THROW(moderation.moderate(hod(),version.submissionId,80.0," "),
                 std::invalid_argument);
    const auto existing=users.findResult(version.submissionId);
    ASSERT_TRUE(existing.has_value());
    EXPECT_FALSE(existing->approved);
    EXPECT_DOUBLE_EQ(existing->earnedMark,75);
    EXPECT_TRUE(users.listMarkAudits(existing->resultId).empty());
}

TEST_F(ModerationTests, HoDCannotAdjustOutsideAssignmentMaximum)
{
    const auto version=markedPastDue();
    EXPECT_THROW(moderation.moderate(hod(),version.submissionId,101.0,"Revised"),
                 std::invalid_argument);
    EXPECT_THROW(moderation.moderate(hod(),version.submissionId,-2.0,"Revised"),
                 std::invalid_argument);
    EXPECT_FALSE(users.findResult(version.submissionId)->approved);
}

TEST_F(ModerationTests, AnotherDepartmentHoDCannotSeeApproveReleaseOrReadAttachments)
{
    const auto version=markedPastDue();
    const auto admin=authentication.logIn("admin1","LongSecretPassword123!");
    const auto dep=management.createDepartment(admin,"Another Department");
    const auto otherHod=management.createUser(admin,"hod2","DepartmentPass456!",
                                              lms::Role::HeadOfDepartment);
    management.assignHoD(admin,otherHod.id,dep.id);
    const auto other=authentication.logIn("hod2","DepartmentPass456!");
    EXPECT_TRUE(moderation.listDepartmentResults(other).empty());
    EXPECT_THROW(moderation.moderate(other,version.submissionId,std::nullopt,""),
                 lms::AuthenticationError);
    EXPECT_THROW(moderation.release(other,version.submissionId),lms::AuthenticationError);
    EXPECT_THROW(moderation.readLatestAttachment(other,version.submissionId),
                 lms::AuthenticationError);
}

TEST_F(ModerationTests, StudentsLecturersAndAdminCannotModerateOrRelease)
{
    const auto version=markedPastDue();
    const auto admin=authentication.logIn("admin1","LongSecretPassword123!");
    EXPECT_THROW(moderation.listDepartmentResults(admin),lms::AuthenticationError);
    EXPECT_THROW(moderation.moderate(studentSession(),version.submissionId,std::nullopt,""),
                 lms::AuthenticationError);
    EXPECT_THROW(moderation.release(lecturer(),version.submissionId),
                 lms::AuthenticationError);
    EXPECT_THROW(moderation.getReleasedResults(hod()),lms::AuthenticationError);
}

TEST_F(ModerationTests, FutureEffectiveDeadlineBlocksApprovalAndRelease)
{
    initialise();
    makeDepartmentHod();
    const auto assignment=makeAssignment("2030-11-01T12:00:00Z");
    const auto version=submissionService.submit(studentSession(),assignment.id,{"Early submission",""});
    marking.mark(lecturer(),version.submissionId,70,"Good","");
    EXPECT_THROW(moderation.moderate(hod(),version.submissionId,std::nullopt,""),
                 std::invalid_argument);
    EXPECT_FALSE(users.findResult(version.submissionId)->approved);
    EXPECT_TRUE(moderation.getReleasedResults(studentSession()).empty());
    EXPECT_THROW(moderation.release(hod(),version.submissionId),
                 std::invalid_argument);
}

TEST_F(ModerationTests, NewerUnmarkedVersionBlocksApproval)
{
    initialise();
    makeDepartmentHod();
    const auto assignment=makeAssignment("2000-01-01T12:00:00Z");
    const auto first=users.saveVersion(studentSession().userId(),assignment.id,
        "draft","","2000-01-01T10:00:00Z");
    marking.mark(lecturer(),first.submissionId,75,"Draft graded","");
    const auto second=users.saveVersion(studentSession().userId(),assignment.id,
        "revision","","2000-01-01T11:00:00Z");
    EXPECT_NE(first.id,second.id);
    EXPECT_THROW(moderation.moderate(hod(),first.submissionId,std::nullopt,""),
                 std::invalid_argument);
    EXPECT_FALSE(users.findResult(first.submissionId)->approved);
    marking.mark(lecturer(),first.submissionId,80,
                 "New version marked","Student resubmitted");
    EXPECT_NO_THROW(moderation.moderate(hod(),first.submissionId,std::nullopt,""));
}

TEST_F(ModerationTests, LecturerCannotEditAnApprovedResult)
{
    const auto version=markedPastDue();
    moderation.moderate(hod(),version.submissionId,std::nullopt,"");
    EXPECT_THROW(marking.mark(lecturer(),version.submissionId,82,
                 "Trying to modify approved result","Wrong"),
                 std::invalid_argument);
}

TEST_F(ModerationTests, HoDCanReviewOnlyAuthorisedAttachment)
{
    initialise();
    makeDepartmentHod();
    const auto assignment=makeAssignment("2000-01-01T12:00:00Z");
    const auto file=testFilesDir/"review.txt";
    {
        std::ofstream stream(file,std::ios::binary);
        stream<<"Assessment attachment";
    }
    const auto staged=store.stage(file.string());
    const auto stored=store.commit(staged);
    const auto version=users.saveVersion(studentSession().userId(),assignment.id,
        "",stored.token,"2000-01-01T12:00:01Z");
    marking.mark(lecturer(),version.submissionId,75,"File assessed","");
    EXPECT_EQ(moderation.readLatestAttachment(hod(),version.submissionId),
              "Assessment attachment");
}


TEST_F(ModerationTests, EffectiveExtensionPreventsPrematureApproval)
{
    const auto version=markedPastDue();
    submissions.grantExtension(lecturer(),
        users.findMarkableSubmission(version.submissionId)->assignmentId,
        studentSession().userId(),"2030-11-01T12:00:00Z");
    EXPECT_THROW(moderation.moderate(hod(),version.submissionId,std::nullopt,""),
                 std::invalid_argument);
    EXPECT_FALSE(users.findResult(version.submissionId)->approved);
    EXPECT_TRUE(moderation.getReleasedResults(studentSession()).empty());
}

TEST_F(ModerationTests, FailedAuditInsertRollsBackHoDMarkAndApproval)
{
    const auto version=markedPastDue();
    ASSERT_EQ(sqlite3_exec(database.connection(),
        "CREATE TRIGGER reject_mark_audit BEFORE INSERT ON mark_change_audits "
        "BEGIN SELECT RAISE(ABORT, 'audit storage unavailable'); END;",
        nullptr,nullptr,nullptr),SQLITE_OK);
    EXPECT_THROW(moderation.moderate(hod(),version.submissionId,80.0,
                                   "Correction requires audit"),std::runtime_error);
    const auto current=users.findResult(version.submissionId);
    ASSERT_TRUE(current.has_value());
    EXPECT_DOUBLE_EQ(current->earnedMark,75);
    EXPECT_DOUBLE_EQ(current->finalMark,65);
    EXPECT_FALSE(current->approved);
    EXPECT_TRUE(users.listMarkAudits(current->resultId).empty());
    const auto submission=users.findMarkableSubmission(version.submissionId);
    EXPECT_EQ(submission->state,"Marked");
}
