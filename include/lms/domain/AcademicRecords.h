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
} // namespace lms
