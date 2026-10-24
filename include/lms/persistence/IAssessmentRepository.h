#pragma once
#include "lms/domain/AssessmentRecords.h"
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace lms {
class IAssessmentRepository {
public:
    virtual ~IAssessmentRepository()=default;
    virtual std::vector<MarkableSubmission> listAssignmentSubmissions(
        std::int64_t assignmentId) const=0;
    virtual std::optional<MarkableSubmission> findMarkableSubmission(
        std::int64_t submissionId) const=0;
    virtual std::optional<AssessmentResult> findResult(std::int64_t submissionId) const=0;
    virtual AssessmentResult saveUnreleasedMark(
        std::int64_t submissionId, std::int64_t expectedVersionId,
        std::int64_t lecturerId, double earnedMark, double penaltyMark,
        double finalMark, std::string_view feedback,
        std::string_view revisionReason, std::string_view changedAtUtc)=0;
    virtual std::vector<MarkChangeAudit> listMarkAudits(std::int64_t resultId) const=0;
};
} // namespace lms
