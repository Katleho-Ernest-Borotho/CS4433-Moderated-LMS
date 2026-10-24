#include "lms/application/MarkingService.h"
#include <algorithm>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace lms {
namespace {
std::string utcNow()
{
    const std::time_t now=std::time(nullptr);
    std::tm utc{};
#ifdef _WIN32
    gmtime_s(&utc,&now);
#else
    gmtime_r(&now,&utc);
#endif
    std::ostringstream ss;
    ss<<std::put_time(&utc,"%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}
}

MarkingService::MarkingService(const IAuthenticationService& authentication,
    const IAcademicRepository& academic, IAssessmentRepository& results,
    const ILatePenaltyPolicy& penalties)
    : authentication_(authentication),academic_(academic),
      results_(results),penalties_(penalties) {}

void MarkingService::requireAssignedLecturer(
    const Session& actor, std::int64_t assignmentId) const
{
    authentication_.requireRole(actor,Role::Lecturer);
    const auto assignment=academic_.findAssignmentById(assignmentId);
    if(!assignment)throw std::invalid_argument("Assignment does not exist");
    if(!academic_.isLecturerAssigned(actor.userId(),assignment->courseId))
        throw AuthenticationError("You are not assigned to this assignment's course");
}

std::vector<MarkableSubmission> MarkingService::listSubmissions(
    const Session& lecturer,std::int64_t assignmentId) const
{
    requireAssignedLecturer(lecturer,assignmentId);
    return results_.listAssignmentSubmissions(assignmentId);
}

AssessmentResult MarkingService::mark(
    const Session& lecturer,std::int64_t submissionId,
    double earnedMark,std::string_view feedback,std::string_view revisionReason)
{
    authentication_.requireRole(lecturer,Role::Lecturer);
    const auto submitted=results_.findMarkableSubmission(submissionId);
    if(!submitted)throw std::invalid_argument("Submission not found");
    requireAssignedLecturer(lecturer,submitted->assignmentId);
    const auto assignment=academic_.findAssignmentById(submitted->assignmentId);
    if(!assignment)throw std::invalid_argument("Assignment no longer exists");
    if(!std::isfinite(earnedMark) || earnedMark<0 || earnedMark>assignment->maximumMark)
        throw std::invalid_argument("Earned mark must be within the assignment maximum");
    if(feedback.empty() || feedback.size()>10000 ||
       feedback.find_first_not_of(" \t\r\n")==std::string_view::npos)
        throw std::invalid_argument("Enter written marking feedback");
    if(revisionReason.size()>1000)
        throw std::invalid_argument("Revision reason is too long");
    const std::string effective=academic_.findEffectiveDeadline(
        submitted->assignmentId,submitted->studentId);
    const auto breakdown=penalties_.calculate(earnedMark,assignment->maximumMark,
        submitted->submittedAtUtc,effective);
    const double roundedEarned=std::round(earnedMark*100.0)/100.0;
    return results_.saveUnreleasedMark(submissionId,submitted->latestVersionId,
        lecturer.userId(),roundedEarned,breakdown.penaltyMark,
        breakdown.finalMark,feedback,revisionReason,utcNow());
}

std::vector<MarkChangeAudit> MarkingService::auditsForSubmission(
    const Session& lecturer,std::int64_t submissionId) const
{
    authentication_.requireRole(lecturer,Role::Lecturer);
    const auto submitted=results_.findMarkableSubmission(submissionId);
    if(!submitted)throw std::invalid_argument("Submission not found");
    requireAssignedLecturer(lecturer,submitted->assignmentId);
    const auto result=results_.findResult(submissionId);
    if(!result)return {};
    return results_.listMarkAudits(result->resultId);
}
} // namespace lms
