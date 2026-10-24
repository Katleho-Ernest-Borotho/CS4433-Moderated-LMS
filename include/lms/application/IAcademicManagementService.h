#pragma once

#include "lms/application/IAuthenticationService.h"
#include "lms/domain/UserAccount.h"
#include <cstdint>
#include <string_view>
#include <vector>

namespace lms {
// Week 3 Academic Management service: currently only user-account operations.
class IAcademicManagementService {
public:
    virtual ~IAcademicManagementService() = default;
    virtual UserAccount createUser(const Session& actor, std::string_view username,
                                   std::string_view initialPassword, Role role) = 0;
    virtual std::vector<UserAccount> listUsers(const Session& actor) const = 0;
    virtual void deactivateUser(const Session& actor, std::int64_t userId) = 0;
};
} // namespace lms
