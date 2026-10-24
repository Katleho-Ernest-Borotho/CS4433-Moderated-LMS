#pragma once
#include "lms/domain/ModerationRecords.h"
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace lms {
class IModerationRepository {
public:
    virtual ~IModerationRepository()=default;
    virtual std::vector<ModerationRecord> listDepartmentResults(std::int64_t hodId) const=0;
    // Repository revalidates department, current marked version, deadline and approval
    // inside its transaction, rather than trusting a previous GUI selection.
    virtual void approveMark(std::int64_t hodId,std::int64_t submissionId,
        std::int64_t expectedVersionId,double earned,double penalty,double finalMark,
        std::string_view reason,std::string_view nowUtc)=0;
    virtual void releaseMark(std::int64_t hodId,std::int64_t submissionId,
        std::int64_t expectedVersionId,std::string_view nowUtc)=0;
    virtual std::vector<ReleasedResult> listReleasedForStudent(std::int64_t studentId) const=0;
};
} // namespace lms
