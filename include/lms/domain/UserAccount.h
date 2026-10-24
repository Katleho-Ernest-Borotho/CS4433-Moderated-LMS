#pragma once

#include "lms/domain/User.h"
#include <cstdint>
#include <string>

namespace lms {
// Account summary: never exposes password hashes to the GUI.
struct UserAccount {
    std::int64_t id = 0;
    std::string username;
    Role role = Role::Student;
    bool active = false;
};
} // namespace lms
