#pragma once

#include "lms/persistence/IUserRepository.h"
#include "lms/persistence/SqliteDatabase.h"

namespace lms {
// Partial SQLiteRecordRepository implementation for User account management.
class SQLiteRecordRepository final : public IUserRepository {
public:
    explicit SQLiteRecordRepository(SqliteDatabase& database);
    std::unique_ptr<User> findByUsername(std::string_view username) const override;
    bool hasUsers() const override;
    void bootstrapAdministrator(std::string_view username, std::string_view passwordHash) override;
    std::int64_t saveUser(std::string_view username, std::string_view passwordHash, Role role) override;
    std::vector<UserAccount> listUsers() const override;
    void deactivateUser(std::int64_t id) override;
private:
    SqliteDatabase& database_;
};
} // namespace lms
