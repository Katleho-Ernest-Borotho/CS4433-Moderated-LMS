#pragma once

#include "lms/persistence/IAcademicRepository.h"
#include "lms/persistence/IUserRepository.h"
#include "lms/persistence/SqliteDatabase.h"

namespace lms {
class SQLiteRecordRepository final : public IUserRepository, public IAcademicRepository {
public:
    explicit SQLiteRecordRepository(SqliteDatabase& database);
    std::unique_ptr<User> findByUsername(std::string_view username) const override;
    bool hasUsers() const override;
    void bootstrapAdministrator(std::string_view username, std::string_view passwordHash) override;
    std::int64_t saveUser(std::string_view username, std::string_view passwordHash, Role role) override;
    std::vector<UserAccount> listUsers() const override;
    void deactivateUser(std::int64_t id) override;

    Department saveDepartment(std::string_view name) override;
    void updateDepartment(const Department& department) override;
    std::vector<Department> listDepartments() const override;
    Course saveCourse(std::int64_t departmentId, std::string_view code,
                      std::string_view title) override;
    void updateCourse(const Course& course) override;
    std::vector<Course> listAllCourses() const override;
    void assignHoD(std::int64_t hodId, std::int64_t departmentId) override;
    void assignLecturer(std::int64_t lecturerId, std::int64_t courseId) override;
    void enrolStudent(std::int64_t studentId, std::int64_t courseId,
                      std::string_view enrolledAtUtc) override;
    std::vector<Course> listStudentCourses(std::int64_t studentId) const override;
    std::vector<Course> listLecturerCourses(std::int64_t lecturerId) const override;
    std::vector<Course> listHoDCourses(std::int64_t hodId) const override;
private:
    SqliteDatabase& database_;
};
} // namespace lms
