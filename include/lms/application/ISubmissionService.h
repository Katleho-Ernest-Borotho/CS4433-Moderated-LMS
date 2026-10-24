#pragma once
#include "lms/application/IAuthenticationService.h"
#include <cstdint>
#include <string>
#include <string_view>

namespace lms {
// Partial Week 3 submission-service contract: extensions and deadlines only.
// Submission content and version operations will be implemented next.
class ISubmissionService {
public:
    virtual ~ISubmissionService() = default;
    virtual void grantExtension(const Session& actor, std::int64_t assignmentId,
                                std::int64_t studentId, std::string_view newDeadlineUtc) = 0;
    virtual std::string effectiveDeadline(const Session& actor, std::int64_t assignmentId,
                                          std::int64_t studentId) const = 0;
};
} // namespace lms
