#pragma once

#include <algorithm>
#include <stdexcept>
#include <string_view>

namespace lms {
inline void validateCredentialsForCreation(std::string_view username, std::string_view password)
{
    const bool validUsername = username.size() >= 3 && username.size() <= 40 &&
        std::all_of(username.begin(), username.end(), [](char c) {
            return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
                || (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '_';
        });
    if (!validUsername) {
        throw std::invalid_argument("Username must contain 3-40 letters, digits, . _ or -");
    }
    if (password.size() < 12 || password.size() > 1024) {
        throw std::invalid_argument("Password must contain 12 to 1024 UTF-8 bytes");
    }
}
} // namespace lms
