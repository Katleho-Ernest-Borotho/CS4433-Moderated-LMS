#pragma once

#include "lms/application/IAuthenticationService.h"
#include "lms/domain/AcademicRecords.h"
#include "lms/domain/UserAccount.h"
#include <cstdint>
#include <string_view>
#include <vector>

namespace lms {
class IAcademicManagementService {
public:
    virtual ~IAcademicManagementService() = default;
    virtual UserAccount createUser(const Session& actor, std::string_view username,
                                   std::string_view initialPassword, Role role) = 0;
    virtual std::vector<UserAccount> listUsers(const Session& actor) const = 0;
    virtual void deactivateUser(const Session& actor, std::int64_t userId) = 0;

    virtual Department createDepartment(const Session& actor, std::string_view name) = 0;
    virtual void updateDepartment(const Session& actor, const Department& department) = 0;
    virtual std::vector<Department> listDepartments(const Session& actor) const = 0;
    virtual Course createCourse(const Session& actor, std::int64_t departmentId,
                                std::string_view code, std::string_view title) = 0;
    virtual void updateCourse(const Session& actor, const Course& course) = 0;
    virtual std::vector<Course> listAllCourses(const Session& actor) const = 0;
    virtual void assignHoD(const Session& actor, std::int64_t hodId,
                           std::int64_t departmentId) = 0;
    virtual void assignLecturer(const Session& actor, std::int64_t lecturerId,
                                std::int64_t courseId) = 0;
    virtual void enrolStudent(const Session& actor, std::int64_t studentId,
                              std::int64_t courseId) = 0;
    virtual std::vector<Course> listVisibleCourses(const Session& actor) const = 0;
    virtual Assignment createAssignment(const Session& actor, std::int64_t courseId,
                                        std::string_view title, std::string_view deadlineUtc,
                                        double maximumMark) = 0;
    virtual std::vector<Assignment> listCourseAssignments(const Session& actor,
                                                           std::int64_t courseId) const = 0;
    virtual std::vector<UserAccount> listEnrolledStudentsForCourse(const Session& actor,
                                                                    std::int64_t courseId) const = 0;
};
} // namespace lms
