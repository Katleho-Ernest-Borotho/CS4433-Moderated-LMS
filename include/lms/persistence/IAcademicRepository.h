#pragma once

#include "lms/domain/AcademicRecords.h"
#include "lms/domain/UserAccount.h"
#include <optional>
#include <string>
#include <cstdint>
#include <string_view>
#include <vector>

namespace lms {
class IAcademicRepository {
public:
    virtual ~IAcademicRepository() = default;
    virtual Department saveDepartment(std::string_view name) = 0;
    virtual void updateDepartment(const Department& department) = 0;
    virtual std::vector<Department> listDepartments() const = 0;
    virtual Course saveCourse(std::int64_t departmentId, std::string_view code,
                              std::string_view title) = 0;
    virtual void updateCourse(const Course& course) = 0;
    virtual std::vector<Course> listAllCourses() const = 0;
    virtual void assignHoD(std::int64_t hodId, std::int64_t departmentId) = 0;
    virtual void assignLecturer(std::int64_t lecturerId, std::int64_t courseId) = 0;
    virtual void enrolStudent(std::int64_t studentId, std::int64_t courseId,
                              std::string_view enrolledAtUtc) = 0;
    virtual std::vector<Course> listStudentCourses(std::int64_t studentId) const = 0;
    virtual std::vector<Course> listLecturerCourses(std::int64_t lecturerId) const = 0;
    virtual std::vector<Course> listHoDCourses(std::int64_t hodId) const = 0;

    virtual bool isLecturerAssigned(std::int64_t lecturerId, std::int64_t courseId) const = 0;
    virtual bool isStudentEnrolled(std::int64_t studentId, std::int64_t courseId) const = 0;
    virtual std::vector<UserAccount> listEnrolledStudents(std::int64_t courseId) const = 0;
    virtual Assignment saveAssignment(std::int64_t courseId, std::string_view title,
                                      std::string_view deadlineUtc, double maximumMark) = 0;
    virtual std::optional<Assignment> findAssignmentById(std::int64_t assignmentId) const = 0;
    virtual std::vector<Assignment> listCourseAssignments(std::int64_t courseId) const = 0;
    virtual void saveExtension(std::int64_t assignmentId, std::int64_t studentId,
                               std::int64_t lecturerId, std::string_view newDeadlineUtc) = 0;
    virtual std::string findEffectiveDeadline(std::int64_t assignmentId,
                                              std::int64_t studentId) const = 0;
};
} // namespace lms
