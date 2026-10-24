#include "lms/application/BootstrapService.h"

#include <algorithm>
#include <stdexcept>

namespace lms {
BootstrapService::BootstrapService(IUserRepository& users, const IPasswordHasher& passwords)
    : users_(users), passwords_(passwords) {}

bool BootstrapService::needsSetup() const { return !users_.hasUsers(); }

void BootstrapService::createInitialAdministrator(std::string_view username,
                                                   std::string_view password)
{
    const bool validUsername = username.size() >= 3 && username.size() <= 40 &&
        std::all_of(username.begin(), username.end(), [](char c) {
            return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
                || (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '_';
        });
    if (!validUsername) {
        throw std::invalid_argument("Username: 3-40 letters, numbers, dots, dashes or underscores");
    }
    if (password.size() < 12 || password.size() > 1024) {
        throw std::invalid_argument("Password must contain 12 to 1024 characters (UTF-8 bytes)");
    }
    const std::string storedHash = passwords_.hash(password);
    users_.bootstrapAdministrator(username, storedHash);
}
} // namespace lms
