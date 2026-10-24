#pragma once

#include "lms/domain/User.h"
#include <cstdint>
#include <memory>
#include <string_view>

namespace lms {
class IUserRepository {
public:
    virtual ~IUserRepository() = default;
    // nullptr represents the optional<User> result from the Week 3 design.
    virtual std::unique_ptr<User> findByUsername(std::string_view username) const = 0;
    virtual bool hasUsers() const = 0;
    // Called only by first-run provisioning, never by the login form directly.
    virtual void bootstrapAdministrator(std::string_view username, std::string_view passwordHash) = 0;
    virtual void deactivateUser(std::int64_t id) = 0;
};
} // namespace lms
