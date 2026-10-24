#include "lms/application/AcademicManagementService.h"
#include "lms/application/AccountValidation.h"

#include <stdexcept>
#include <string>

namespace lms {

AcademicManagementService::AcademicManagementService(
    const IAuthenticationService& authentication,
    IUserRepository& users,
    const IPasswordHasher& passwords)
    : authentication_(authentication), users_(users), passwords_(passwords) {}

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
    if (actor.userId() == userId) {
        throw std::invalid_argument("You cannot deactivate your own administrator account");
    }
    users_.deactivateUser(userId);
}
} // namespace lms
