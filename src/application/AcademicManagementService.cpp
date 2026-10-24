#include "lms/application/AcademicManagementService.h"
#include "lms/application/AccountValidation.h"
#include "lms/domain/UtcDateTime.h"
#include <algorithm>
#include <cmath>

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

namespace lms {
namespace {
void ensureText(std::string_view value, std::string_view field, std::size_t maxBytes)
{
    if (value.empty() || value.size() > maxBytes || value.find_first_not_of(" \t\r\n") == std::string_view::npos) {
        throw std::invalid_argument(std::string(field) + " must be non-empty and at most " +
                                    std::to_string(maxBytes) + " bytes");
    }
}

std::string utcNow()
{
    const std::time_t current = std::time(nullptr);
    std::tm utc{};
#ifdef _WIN32
    gmtime_s(&utc, &current);
#else
    gmtime_r(&current, &utc);
#endif
    std::ostringstream result;
    result << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    return result.str();
}
} // namespace

AcademicManagementService::AcademicManagementService(
    const IAuthenticationService& authentication, IUserRepository& users,
    IAcademicRepository& academic, const IPasswordHasher& passwords)
    : authentication_(authentication), users_(users), academic_(academic),
      passwords_(passwords) {}

UserAccount AcademicManagementService::createUser(
    const Session& actor, std::string_view username,
    std::string_view initialPassword, Role role)
{
    authentication_.requireRole(actor, Role::Administrator);
    validateCredentialsForCreation(username, initialPassword);
    const std::string storedHash = passwords_.hash(initialPassword);
    const auto id = users_.saveUser(username, storedHash, role);
    return UserAccount{id, std::string(username), role, true};
}

std::vector<UserAccount> AcademicManagementService::listUsers(const Session& actor) const
{
    authentication_.requireRole(actor, Role::Administrator);
    return users_.listUsers();
}

void AcademicManagementService::deactivateUser(const Session& actor, std::int64_t userId)
{
    authentication_.requireRole(actor, Role::Administrator);
    if (actor.userId() == userId)
        throw std::invalid_argument("You cannot deactivate your own administrator account");
    users_.deactivateUser(userId);
}

Department AcademicManagementService::createDepartment(const Session& actor, std::string_view name)
{
    authentication_.requireRole(actor, Role::Administrator);
    ensureText(name, "Department name", 120);
    return academic_.saveDepartment(name);
}

void AcademicManagementService::updateDepartment(const Session& actor, const Department& department)
{
    authentication_.requireRole(actor, Role::Administrator);
    ensureText(department.name, "Department name", 120);
    academic_.updateDepartment(department);
}

std::vector<Department> AcademicManagementService::listDepartments(const Session& actor) const
{
    authentication_.requireRole(actor, Role::Administrator);
    return academic_.listDepartments();
}

Course AcademicManagementService::createCourse(const Session& actor, std::int64_t departmentId,
                                               std::string_view code, std::string_view title)
{
    authentication_.requireRole(actor, Role::Administrator);
    ensureText(code, "Course code", 30);
    ensureText(title, "Course title", 150);
    if (departmentId <= 0) throw std::invalid_argument("Select a department");
    return academic_.saveCourse(departmentId, code, title);
}

void AcademicManagementService::updateCourse(const Session& actor, const Course& course)
{
    authentication_.requireRole(actor, Role::Administrator);
    ensureText(course.code, "Course code", 30);
    ensureText(course.title, "Course title", 150);
    if (course.departmentId <= 0) throw std::invalid_argument("Select a department");
    academic_.updateCourse(course);
}

std::vector<Course> AcademicManagementService::listAllCourses(const Session& actor) const
{
    authentication_.requireRole(actor, Role::Administrator);
    return academic_.listAllCourses();
}

void AcademicManagementService::assignHoD(const Session& actor, std::int64_t hodId,
                                          std::int64_t departmentId)
{
    authentication_.requireRole(actor, Role::Administrator);
    academic_.assignHoD(hodId, departmentId);
}

void AcademicManagementService::assignLecturer(const Session& actor, std::int64_t lecturerId,
                                               std::int64_t courseId)
{
    authentication_.requireRole(actor, Role::Administrator);
    academic_.assignLecturer(lecturerId, courseId);
}

void AcademicManagementService::enrolStudent(const Session& actor, std::int64_t studentId,
                                             std::int64_t courseId)
{
    authentication_.requireRole(actor, Role::Administrator);
    academic_.enrolStudent(studentId, courseId, utcNow());
}

std::vector<Course> AcademicManagementService::listVisibleCourses(const Session& actor) const
{
    switch (actor.role()) {
    case Role::Student:
        authentication_.requireRole(actor, Role::Student);
        return academic_.listStudentCourses(actor.userId());
    case Role::Lecturer:
        authentication_.requireRole(actor, Role::Lecturer);
        return academic_.listLecturerCourses(actor.userId());
    case Role::HeadOfDepartment:
        authentication_.requireRole(actor, Role::HeadOfDepartment);
        return academic_.listHoDCourses(actor.userId());
    case Role::Administrator:
        authentication_.requireRole(actor, Role::Administrator);
        return academic_.listAllCourses();
    }
    throw AuthenticationError("Access denied");
}

Assignment AcademicManagementService::createAssignment(
    const Session& actor, std::int64_t courseId, std::string_view title,
    std::string_view deadlineUtc, double maximumMark)
{
    authentication_.requireRole(actor, Role::Lecturer);
    if (!academic_.isLecturerAssigned(actor.userId(), courseId))
        throw AuthenticationError("You are not assigned to this course");
    ensureText(title, "Assignment title", 160);
    validateUtcDateTime(deadlineUtc);
    if (!std::isfinite(maximumMark) || maximumMark <= 0.0 || maximumMark > 1000000.0)
        throw std::invalid_argument("Maximum mark must be positive and finite");
    return academic_.saveAssignment(courseId, title, deadlineUtc, maximumMark);
}

std::vector<Assignment> AcademicManagementService::listCourseAssignments(
    const Session& actor, std::int64_t courseId) const
{
    switch (actor.role()) {
    case Role::Lecturer:
        authentication_.requireRole(actor, Role::Lecturer);
        if (!academic_.isLecturerAssigned(actor.userId(), courseId))
            throw AuthenticationError("Course not assigned to lecturer");
        break;
    case Role::Student:
        authentication_.requireRole(actor, Role::Student);
        if (!academic_.isStudentEnrolled(actor.userId(), courseId))
            throw AuthenticationError("Student not enrolled in course");
        break;
    case Role::Administrator: {
        authentication_.requireRole(actor, Role::Administrator);
        const auto all = academic_.listAllCourses();
        if (std::none_of(all.begin(), all.end(), [courseId](const Course& c) {
            return c.id == courseId;
        })) throw std::invalid_argument("Course not found");
        break;
    }
    case Role::HeadOfDepartment:
        authentication_.requireRole(actor, Role::HeadOfDepartment);
        {
            const auto own = academic_.listHoDCourses(actor.userId());
            if (std::none_of(own.begin(), own.end(), [courseId](const Course& c) {
                return c.id == courseId;
            })) throw AuthenticationError("Course outside your department");
        }
        break;
    }
    return academic_.listCourseAssignments(courseId);
}

std::vector<UserAccount> AcademicManagementService::listEnrolledStudentsForCourse(
    const Session& actor, std::int64_t courseId) const
{
    authentication_.requireRole(actor, Role::Lecturer);
    if (!academic_.isLecturerAssigned(actor.userId(), courseId))
        throw AuthenticationError("Course not assigned to lecturer");
    return academic_.listEnrolledStudents(courseId);
}
} // namespace lms
