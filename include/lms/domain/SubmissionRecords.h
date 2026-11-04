#pragma once
#include <cstdint>
#include <string>

namespace lms {
struct SubmissionContent {
    std::string text;
    std::string attachmentPath; // Empty for a text-only submission.
};
struct SubmissionVersion {
    std::int64_t id = 0;
    std::int64_t submissionId = 0;
    int versionNumber = 0;
    std::string submittedAtUtc;
    std::string text;
    std::string attachmentRef; // Opaque internal file reference, not a filesystem path.
    bool late = false;
};
struct SubmissionSummary {
    std::int64_t id = 0;
    std::int64_t studentId = 0;
    std::int64_t assignmentId = 0;
    std::string state;
};
} // namespace lms
