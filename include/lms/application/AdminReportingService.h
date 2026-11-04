#pragma once
#include "lms/application/IAuthenticationService.h"
#include <sqlite3.h>
#include <cstdint>
#include <string>
#include <vector>

namespace lms {
struct CourseReportRow {
    std::string code, title, department;
    std::int64_t enrolled=0, assignments=0, submissions=0, releasedResults=0;
};
struct AdminReport {
    std::int64_t activeUsers=0, inactiveUsers=0, departments=0, courses=0;
    std::int64_t assignments=0, submissions=0, versions=0, provisional=0;
    std::int64_t approvedUnreleased=0, released=0;
    std::vector<CourseReportRow> courseRows;
};
class AdminReportingService final {
public:
    AdminReportingService(const IAuthenticationService& authentication,sqlite3* database);
    AdminReport generate(const Session& administrator) const;
    static std::string toCsv(const AdminReport& report);
private:
    const IAuthenticationService& authentication_;
    sqlite3* db_=nullptr;
};
} // namespace lms
