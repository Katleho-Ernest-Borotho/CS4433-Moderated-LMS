#pragma once
#include "lms/application/IMarkingService.h"
#include "lms/domain/LatePenaltyPolicy.h"
#include "lms/persistence/IAcademicRepository.h"
#include "lms/persistence/IAssessmentRepository.h"

namespace lms {
class MarkingService final : public IMarkingService {
public:
    MarkingService(const IAuthenticationService& authentication,
        const IAcademicRepository& academic, IAssessmentRepository& results,
        const ILatePenaltyPolicy& penalties);
    std::vector<MarkableSubmission> listSubmissions(
        const Session& lecturer, std::int64_t assignmentId) const override;
    AssessmentResult mark(const Session& lecturer, std::int64_t submissionId,
        double earnedMark, std::string_view feedback,
        std::string_view revisionReason) override;
    std::vector<MarkChangeAudit> auditsForSubmission(
        const Session& lecturer, std::int64_t submissionId) const override;
private:
    void requireAssignedLecturer(const Session& actor, std::int64_t assignmentId) const;
    const IAuthenticationService& authentication_;
    const IAcademicRepository& academic_;
    IAssessmentRepository& results_;
    const ILatePenaltyPolicy& penalties_;
};
} // namespace lms
