#pragma once

#include "lms/persistence/IUserRepository.h"
#include "lms/persistence/SqliteDatabase.h"

namespace lms {
// The first part of the Week 3 SQLiteRecordRepository: user account operations.
class SQLiteRecordRepository final : public IUserRepository {
public:
    explicit SQLiteRecordRepository(SqliteDatabase& database);
    std::unique_ptr<User> findByUsername(std::string_view username) const override;
    bool hasUsers() const override;
    void bootstrapAdministrator(std::string_view username, std::string_view passwordHash) override;
    void deactivateUser(std::int64_t id) override;
private:
    SqliteDatabase& database_;
};
} // namespace lms
