#include "lms/application/AuthenticationService.h"

namespace lms {
AuthenticationService::AuthenticationService(const IUserRepository& users,
                                             const IPasswordHasher& passwords)
    : users_(users), passwords_(passwords) {}

Session AuthenticationService::logIn(std::string_view username, std::string_view password) const
{
    // Same visible error for unknown, incorrect and inactive accounts.
    if (username.empty() || password.empty()) {
        throw AuthenticationError("Invalid username or password");
    }
    const auto user = users_.findByUsername(username);
    if (!user || !user->active() || !passwords_.verify(password, user->passwordHash())) {
        throw AuthenticationError("Invalid username or password");
    }
    return Session(user->id(), user->role(), user->username());
}

void AuthenticationService::requireRole(const Session& session, Role expected) const
{
    // Revalidate the account so a deactivated user cannot keep using an old session.
    const auto user = users_.findByUsername(session.username());
    if (!user || !user->active() || user->id() != session.userId()
        || user->role() != session.role() || user->role() != expected) {
        throw AuthenticationError("Access denied");
    }
}
} // namespace lms
