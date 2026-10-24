#include "lms/application/BootstrapService.h"
#include "lms/application/AccountValidation.h"

namespace lms {
BootstrapService::BootstrapService(IUserRepository& users, const IPasswordHasher& passwords)
    : users_(users), passwords_(passwords) {}

bool BootstrapService::needsSetup() const { return !users_.hasUsers(); }

void BootstrapService::createInitialAdministrator(std::string_view username,
                                                   std::string_view password)
{
    validateCredentialsForCreation(username, password);
    const std::string storedHash = passwords_.hash(password);
    users_.bootstrapAdministrator(username, storedHash);
}
} // namespace lms
