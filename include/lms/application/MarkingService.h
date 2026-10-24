#pragma once
#include "lms/application/IMarkingService.h"
#include "lms/domain/LatePenaltyPolicy.h"
#include "lms/persistence/IAcademicRepository.h"
#include "lms/persistence/IAssessmentRepository.h"
#include "lms/persistence/ISubmissionContentStore.h"

namespace lms {
class MarkingService final : public IMarkingService {
public:
    MarkingService(const IAuthenticationService& authentication,
        const IAcademicRepository& academic, IAssessmentRepository& results,
        const ILatePenaltyPolicy& penalties, const ISubmissionContentStore& files);
    std::vector<MarkableSubmission> listSubmissions(
        const Session& lecturer, std::int64_t assignmentId) const override;
    AssessmentResult mark(const Session& lecturer, std::int64_t submissionId,
        double earnedMark, std::string_view feedback,
        std::string_view revisionReason) override;
    std::optional<AssessmentResult> currentResult(
        const Session& lecturer, std::int64_t submissionId) const override;
    std::string readLatestAttachment(
        const Session& lecturer, std::int64_t submissionId) const override;
    std::vector<MarkChangeAudit> auditsForSubmission(
        const Session& lecturer, std::int64_t submissionId) const override;
private:
    void requireAssignedLecturer(const Session& actor, std::int64_t assignmentId) const;
    const IAuthenticationService& authentication_;
    const IAcademicRepository& academic_;
    IAssessmentRepository& results_;
    const ILatePenaltyPolicy& penalties_;
    const ISubmissionContentStore& files_;
};
} // namespace lms
