#pragma once

#include "lms/application/IAuthenticationService.h"
#include "lms/persistence/IUserRepository.h"
#include "lms/security/IPasswordHasher.h"

namespace lms {
class AuthenticationService final : public IAuthenticationService {
public:
    AuthenticationService(const IUserRepository& users, const IPasswordHasher& passwords);
    Session logIn(std::string_view username, std::string_view password) const override;
    void requireRole(const Session& session, Role expected) const override;
private:
    const IUserRepository& users_;
    const IPasswordHasher& passwords_;
};
} // namespace lms
