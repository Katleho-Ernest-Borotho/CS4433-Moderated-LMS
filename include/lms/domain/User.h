#pragma once

#include <cstdint>
#include <string>

namespace lms {

enum class Role { Student, Lecturer, HeadOfDepartment, Administrator };

class User {
public:
    User(std::int64_t id, std::string username, std::string passwordHash, bool active = true);
    virtual ~User() = default;

    virtual Role role() const noexcept = 0;

    std::int64_t id() const noexcept;
    const std::string& username() const noexcept;
    const std::string& passwordHash() const noexcept;
    bool active() const noexcept;

private:
    std::int64_t id_;
    std::string username_;
    std::string passwordHash_;
    bool active_;
};

class Student final : public User {
public:
    using User::User;
    Role role() const noexcept override;
};

class Lecturer final : public User {
public:
    using User::User;
    Role role() const noexcept override;
};

class HeadOfDepartment final : public User {
public:
    using User::User;
    Role role() const noexcept override;
};

class Administrator final : public User {
public:
    using User::User;
    Role role() const noexcept override;
};

} // namespace lms
