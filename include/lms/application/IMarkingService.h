#pragma once
#include "lms/application/IAuthenticationService.h"
#include "lms/domain/AssessmentRecords.h"
#include <cstdint>
#include <string_view>
#include <vector>

namespace lms {
class IMarkingService {
public:
    virtual ~IMarkingService()=default;
    virtual std::vector<MarkableSubmission> listSubmissions(
        const Session& lecturer, std::int64_t assignmentId) const=0;
    virtual AssessmentResult mark(const Session& lecturer,
        std::int64_t submissionId, double earnedMark,
        std::string_view feedback, std::string_view revisionReason)=0;
    virtual std::vector<MarkChangeAudit> auditsForSubmission(
        const Session& lecturer, std::int64_t submissionId) const=0;
};
} // namespace lms
