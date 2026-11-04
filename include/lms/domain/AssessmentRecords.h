#pragma once
#include <cstdint>
#include <string>

namespace lms {
struct MarkableSubmission {
    std::int64_t submissionId=0;
    std::int64_t studentId=0;
    std::int64_t assignmentId=0;
    std::int64_t latestVersionId=0;
    int latestVersionNumber=0;
    std::string studentUsername;
    std::string submittedAtUtc;
    std::string text;
    std::string attachmentRef;
    std::string state;
};

struct AssessmentResult {
    std::int64_t resultId=0;
    std::int64_t submissionId=0;
    std::int64_t markedVersionId=0;
    std::int64_t lecturerId=0;
    double earnedMark=0;
    double penaltyMark=0;
    double finalMark=0;
    std::string feedback;
    bool approved=false;
    std::string releasedAtUtc;
};

struct MarkChangeAudit {
    std::int64_t resultId=0;
    std::int64_t actorId=0;
    std::string field;
    double oldValue=0;
    double newValue=0;
    std::string reason;
};
} // namespace lms
