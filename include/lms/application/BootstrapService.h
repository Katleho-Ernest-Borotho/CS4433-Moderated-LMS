#pragma once

#include "lms/persistence/IUserRepository.h"
#include "lms/security/IPasswordHasher.h"
#include <string_view>

namespace lms {
// Only used for first-run provisioning when the database has no accounts.
class BootstrapService final {
public:
    BootstrapService(IUserRepository& users, const IPasswordHasher& passwords);
    bool needsSetup() const;
    void createInitialAdministrator(std::string_view username, std::string_view password);
private:
    IUserRepository& users_;
    const IPasswordHasher& passwords_;
};
} // namespace lms
