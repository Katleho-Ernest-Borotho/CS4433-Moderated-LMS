#pragma once

#include "lms/domain/User.h"
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace lms {
class AuthenticationError : public std::runtime_error {
public:
    explicit AuthenticationError(const char* message) : std::runtime_error(message) {}
};

// A session is issued by the authentication service after checking credentials.
// GUI callers cannot construct a Session directly.
class Session final {
public:
    std::int64_t userId() const noexcept { return userId_; }
    Role role() const noexcept { return role_; }
    const std::string& username() const noexcept { return username_; }
private:
    friend class AuthenticationService;
    Session(std::int64_t userId, Role role, std::string username)
        : userId_(userId), role_(role), username_(std::move(username)) {}
    std::int64_t userId_;
    Role role_;
    std::string username_;
};

class IAuthenticationService {
public:
    virtual ~IAuthenticationService() = default;
    virtual Session logIn(std::string_view username, std::string_view password) const = 0;
    virtual void requireRole(const Session& session, Role expected) const = 0;
};
} // namespace lms
