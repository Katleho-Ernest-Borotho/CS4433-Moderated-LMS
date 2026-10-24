#pragma once
#include "lms/application/ISubmissionService.h"
#include "lms/persistence/IAcademicRepository.h"

namespace lms {
class SubmissionManagementService final : public ISubmissionService {
public:
    SubmissionManagementService(const IAuthenticationService& authentication,
                                IAcademicRepository& academic);
    void grantExtension(const Session& actor, std::int64_t assignmentId,
                        std::int64_t studentId, std::string_view newDeadlineUtc) override;
    std::string effectiveDeadline(const Session& actor, std::int64_t assignmentId,
                                  std::int64_t studentId) const override;
private:
    const IAuthenticationService& authentication_;
    IAcademicRepository& academic_;
};
} // namespace lms
