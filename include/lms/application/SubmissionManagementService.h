#pragma once
#include "lms/application/ISubmissionService.h"
#include "lms/persistence/IAcademicRepository.h"
#include "lms/persistence/ISubmissionRepository.h"
#include "lms/persistence/ISubmissionContentStore.h"

namespace lms {
class SubmissionManagementService final : public ISubmissionService {
public:
    SubmissionManagementService(const IAuthenticationService& authentication,
                                IAcademicRepository& academic);
    SubmissionManagementService(const IAuthenticationService& authentication,
                                IAcademicRepository& academic,
                                ISubmissionRepository& submissions,
                                ISubmissionContentStore& files);
    SubmissionVersion submit(const Session& actor, std::int64_t assignmentId,
                             const SubmissionContent& content) override;
    std::vector<SubmissionVersion> listMyVersions(const Session& actor,
                                                  std::int64_t assignmentId) const override;
    void grantExtension(const Session& actor, std::int64_t assignmentId,
                        std::int64_t studentId, std::string_view newDeadlineUtc) override;
    std::string effectiveDeadline(const Session& actor, std::int64_t assignmentId,
                                  std::int64_t studentId) const override;
private:
    const IAuthenticationService& authentication_;
    IAcademicRepository& academic_;
    ISubmissionRepository* submissions_ = nullptr;
    ISubmissionContentStore* files_ = nullptr;
};
} // namespace lms
