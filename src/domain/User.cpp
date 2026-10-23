#include "lms/domain/User.h"

#include <stdexcept>
#include <utility>

namespace lms {

User::User(std::int64_t id, std::string username, std::string passwordHash, bool active)
    : id_(id),
      username_(std::move(username)),
      passwordHash_(std::move(passwordHash)),
      active_(active)
{
    if (id_ < 0 || username_.empty() || passwordHash_.empty()) {
        throw std::invalid_argument("A user needs an ID, username and password hash");
    }
}

std::int64_t User::id() const noexcept { return id_; }
const std::string& User::username() const noexcept { return username_; }
const std::string& User::passwordHash() const noexcept { return passwordHash_; }
bool User::active() const noexcept { return active_; }

Role Student::role() const noexcept { return Role::Student; }
Role Lecturer::role() const noexcept { return Role::Lecturer; }
Role HeadOfDepartment::role() const noexcept { return Role::HeadOfDepartment; }
Role Administrator::role() const noexcept { return Role::Administrator; }

} // namespace lms
