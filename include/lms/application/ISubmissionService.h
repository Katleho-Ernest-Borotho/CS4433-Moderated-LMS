#pragma once
#include "lms/application/IAuthenticationService.h"
#include "lms/domain/SubmissionRecords.h"
#include <vector>
#include <cstdint>
#include <string>
#include <string_view>

namespace lms {
// Supports first submission, versioned replacements, and individual extensions.
class ISubmissionService {
public:
    virtual ~ISubmissionService() = default;
    virtual SubmissionVersion submit(const Session& actor, std::int64_t assignmentId,
                                      const SubmissionContent& content) = 0;
    virtual std::vector<SubmissionVersion> listMyVersions(const Session& actor,
                                                           std::int64_t assignmentId) const = 0;
    virtual void grantExtension(const Session& actor, std::int64_t assignmentId,
                                std::int64_t studentId, std::string_view newDeadlineUtc) = 0;
    virtual std::string effectiveDeadline(const Session& actor, std::int64_t assignmentId,
                                          std::int64_t studentId) const = 0;
};
} // namespace lms
