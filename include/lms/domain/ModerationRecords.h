#pragma once
#include <cstdint>
#include <string>

namespace lms {
// Internal departmental moderation view, never returned to Student before release.
struct ModerationRecord {
    std::int64_t resultId=0, submissionId=0, assignmentId=0, studentId=0;
    std::int64_t markedVersionId=0, latestVersionId=0;
    std::string studentUsername, courseCode, assignmentTitle;
    std::string latestSubmittedAtUtc, effectiveDeadlineUtc, feedback;
    double maximumMark=0, earnedMark=0, penaltyMark=0, finalMark=0;
    bool approved=false;
    std::string releasedAtUtc;
};
struct ReleasedResult {
    std::int64_t submissionId=0, assignmentId=0;
    std::string courseCode, assignmentTitle, feedback, releasedAtUtc;
    double maximumMark=0, earnedMark=0, penaltyMark=0, finalMark=0;
};
} // namespace lms
