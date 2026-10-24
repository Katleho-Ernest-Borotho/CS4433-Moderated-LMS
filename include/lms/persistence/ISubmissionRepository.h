#pragma once
#include "lms/domain/SubmissionRecords.h"
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace lms {
class ISubmissionRepository {
public:
    virtual ~ISubmissionRepository() = default;
    virtual std::optional<SubmissionSummary> findFor(std::int64_t studentId,
                                                      std::int64_t assignmentId) const = 0;
    virtual SubmissionVersion saveVersion(std::int64_t studentId, std::int64_t assignmentId,
                                           std::string_view text, std::string_view attachmentRef,
                                           std::string_view submittedAtUtc) = 0;
    virtual std::vector<SubmissionVersion> listVersions(std::int64_t studentId,
                                                        std::int64_t assignmentId) const = 0;
};
} // namespace lms
