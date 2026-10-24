#pragma once
#include "lms/application/IModerationService.h"
#include "lms/domain/LatePenaltyPolicy.h"
#include "lms/persistence/IModerationRepository.h"

namespace lms {
class ModerationService final : public IModerationService {
public:
    ModerationService(const IAuthenticationService& auth,IModerationRepository& repository,
                      const ILatePenaltyPolicy& penalties);
    std::vector<ModerationRecord> listDepartmentResults(const Session& hod) const override;
    void moderate(const Session& hod,std::int64_t submissionId,
        std::optional<double> adjustedEarnedMark,std::string_view reason) override;
    void release(const Session& hod,std::int64_t submissionId) override;
    std::vector<ReleasedResult> getReleasedResults(const Session& student) const override;
private:
    const IAuthenticationService& auth_;
    IModerationRepository& repo_;
    const ILatePenaltyPolicy& penalties_;
};
} // namespace lms
