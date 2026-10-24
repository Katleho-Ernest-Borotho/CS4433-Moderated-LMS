#pragma once
#include "lms/application/IAuthenticationService.h"
#include "lms/domain/ModerationRecords.h"
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>
namespace lms {
class IModerationService {
public:
    virtual ~IModerationService()=default;
    virtual std::vector<ModerationRecord> listDepartmentResults(const Session& hod) const=0;
    virtual void moderate(const Session& hod,std::int64_t submissionId,
        std::optional<double> adjustedEarnedMark,std::string_view reason)=0;
    virtual void release(const Session& hod,std::int64_t submissionId)=0;
    virtual std::string readLatestAttachment(const Session& hod,
        std::int64_t submissionId) const=0;
    virtual std::vector<ReleasedResult> getReleasedResults(const Session& student) const=0;
};
} // namespace lms
