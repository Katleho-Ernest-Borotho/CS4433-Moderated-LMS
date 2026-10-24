#include "lms/application/AcademicManagementService.h"
#include "lms/application/AuthenticationService.h"
#include "lms/application/BootstrapService.h"
#include "lms/application/SubmissionManagementService.h"
#include "lms/domain/UtcDateTime.h"
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
