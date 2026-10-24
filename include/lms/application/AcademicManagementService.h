#pragma once

#include "lms/application/IAcademicManagementService.h"
#include "lms/persistence/IAcademicRepository.h"
#include "lms/persistence/IUserRepository.h"
#include "lms/security/IPasswordHasher.h"

namespace lms {
class AcademicManagementService final : public IAcademicManagementService {
public:
    AcademicManagementService(const IAuthenticationService& authentication,
                              IUserRepository& users,
                              IAcademicRepository& academic,
                              const IPasswordHasher& passwords);
    UserAccount createUser(const Session& actor, std::string_view username,
                           std::string_view initialPassword, Role role) override;
    std::vector<UserAccount> listUsers(const Session& actor) const override;
    void deactivateUser(const Session& actor, std::int64_t userId) override;
    Department createDepartment(const Session& actor, std::string_view name) override;
    void updateDepartment(const Session& actor, const Department& department) override;
    std::vector<Department> listDepartments(const Session& actor) const override;
    Course createCourse(const Session& actor, std::int64_t departmentId,
                        std::string_view code, std::string_view title) override;
    void updateCourse(const Session& actor, const Course& course) override;
    std::vector<Course> listAllCourses(const Session& actor) const override;
    void assignHoD(const Session& actor, std::int64_t hodId,
                   std::int64_t departmentId) override;
    void assignLecturer(const Session& actor, std::int64_t lecturerId,
                        std::int64_t courseId) override;
    void enrolStudent(const Session& actor, std::int64_t studentId,
                      std::int64_t courseId) override;
    std::vector<Course> listVisibleCourses(const Session& actor) const override;
private:
    const IAuthenticationService& authentication_;
    IUserRepository& users_;
    IAcademicRepository& academic_;
    const IPasswordHasher& passwords_;
};
} // namespace lms
