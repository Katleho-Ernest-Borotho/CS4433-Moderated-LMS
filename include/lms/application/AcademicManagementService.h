#pragma once

#include "lms/application/IAcademicManagementService.h"
#include "lms/persistence/IUserRepository.h"
#include "lms/security/IPasswordHasher.h"

namespace lms {
class AcademicManagementService final : public IAcademicManagementService {
public:
    AcademicManagementService(const IAuthenticationService& authentication,
                              IUserRepository& users, const IPasswordHasher& passwords);
    UserAccount createUser(const Session& actor, std::string_view username,
                           std::string_view initialPassword, Role role) override;
    std::vector<UserAccount> listUsers(const Session& actor) const override;
    void deactivateUser(const Session& actor, std::int64_t userId) override;
private:
    const IAuthenticationService& authentication_;
    IUserRepository& users_;
    const IPasswordHasher& passwords_;
};
} // namespace lms
