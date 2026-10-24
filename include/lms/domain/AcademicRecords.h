#pragma once
#include <cstdint>
#include <string>

namespace lms {
struct Department {
    std::int64_t id = 0;
    std::string name;
};
struct Course {
    std::int64_t id = 0;
    std::int64_t departmentId = 0;
    std::string code;
    std::string title;
};
struct Assignment {
    std::int64_t id = 0;
    std::int64_t courseId = 0;
    std::string title;
    std::string deadlineUtc;
    double maximumMark = 0.0;
};
struct DeadlineExtension {
    std::int64_t id = 0;
    std::int64_t assignmentId = 0;
    std::int64_t studentId = 0;
    std::int64_t lecturerId = 0;
    std::string newDeadlineUtc;
};
} // namespace lms
