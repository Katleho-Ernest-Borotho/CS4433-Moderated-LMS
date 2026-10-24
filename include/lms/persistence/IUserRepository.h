#pragma once

#include "lms/domain/User.h"
#include "lms/domain/UserAccount.h"
#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

namespace lms {
class IUserRepository {
public:
    virtual ~IUserRepository() = default;
    virtual std::unique_ptr<User> findByUsername(std::string_view username) const = 0;
    virtual bool hasUsers() const = 0;
    virtual void bootstrapAdministrator(std::string_view username, std::string_view passwordHash) = 0;
    virtual std::int64_t saveUser(std::string_view username, std::string_view passwordHash, Role role) = 0;
    virtual std::vector<UserAccount> listUsers() const = 0;
    virtual void deactivateUser(std::int64_t id) = 0;
};
} // namespace lms
