#include "lms/application/ModerationService.h"
#include "lms/domain/UtcDateTime.h"
#include <algorithm>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

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
    std::ostringstream result;
    result<<std::put_time(&utc,"%Y-%m-%dT%H:%M:%SZ");
    return result.str();
}

ModerationRecord findRecord(const std::vector<ModerationRecord>& records,std::int64_t id)
{
    const auto it=std::find_if(records.begin(),records.end(),
        [id](const ModerationRecord& r){return r.submissionId==id;});
    if(it==records.end())throw AuthenticationError("Assessment not found in your department");
    return *it;
}
} // namespace

ModerationService::ModerationService(const IAuthenticationService& auth,
    IModerationRepository& repository,const ILatePenaltyPolicy& penalties,
    const ISubmissionContentStore& files)
    : auth_(auth),repo_(repository),penalties_(penalties),files_(files) {}

std::vector<ModerationRecord> ModerationService::listDepartmentResults(const Session& hod) const
{
    auth_.requireRole(hod,Role::HeadOfDepartment);
    return repo_.listDepartmentResults(hod.userId());
}

void ModerationService::moderate(const Session& hod,std::int64_t submissionId,
    std::optional<double> adjustedEarnedMark,std::string_view reason)
{
    auth_.requireRole(hod,Role::HeadOfDepartment);
    const auto record=findRecord(repo_.listDepartmentResults(hod.userId()),submissionId);
    if(record.approved || !record.releasedAtUtc.empty())
        throw std::invalid_argument("The result has already been approved or released");
    if(record.markedVersionId!=record.latestVersionId)
        throw std::invalid_argument("A newer submission version must be marked first");
    const std::string now=utcNow();
    if(now<=record.effectiveDeadlineUtc)
        throw std::invalid_argument("Moderation is only allowed after the effective deadline");

    double earned=adjustedEarnedMark.value_or(record.earnedMark);
    if(!std::isfinite(earned) || earned<0.0 || earned>record.maximumMark)
        throw std::invalid_argument("Adjusted earned mark is outside assignment maximum");
    earned=std::round(earned*100.0)/100.0;
    const auto breakdown=penalties_.calculate(earned,record.maximumMark,
        record.latestSubmittedAtUtc,record.effectiveDeadlineUtc);
    const bool numbersChange=earned!=record.earnedMark ||
        breakdown.penaltyMark!=record.penaltyMark ||
        breakdown.finalMark!=record.finalMark;
    if(numbersChange && (reason.empty() || reason.size()>1000 ||
       reason.find_first_not_of(" \t\r\n")==std::string_view::npos))
        throw std::invalid_argument("A reason is required for any adjusted mark");
    repo_.approveMark(hod.userId(),submissionId,record.latestVersionId,earned,
        breakdown.penaltyMark,breakdown.finalMark,reason,now);
}

void ModerationService::release(const Session& hod,std::int64_t submissionId)
{
    auth_.requireRole(hod,Role::HeadOfDepartment);
    const auto record=findRecord(repo_.listDepartmentResults(hod.userId()),submissionId);
    if(!record.approved || !record.releasedAtUtc.empty())
        throw std::invalid_argument("Only approved, unreleased results may be released");
    if(record.markedVersionId!=record.latestVersionId)
        throw std::invalid_argument("The latest submission version has not been marked");
    const std::string now=utcNow();
    if(now<=record.effectiveDeadlineUtc)
        throw std::invalid_argument("Results cannot be released before the effective deadline");
    repo_.releaseMark(hod.userId(),submissionId,record.latestVersionId,now);
}

std::string ModerationService::readLatestAttachment(
    const Session& hod,std::int64_t submissionId) const
{
    auth_.requireRole(hod,Role::HeadOfDepartment);
    const auto record=findRecord(repo_.listDepartmentResults(hod.userId()),submissionId);
    if(record.attachmentRef.empty())
        throw std::invalid_argument("There is no attachment on this version");
    return files_.read(StoredRef{record.attachmentRef});
}

std::vector<ReleasedResult> ModerationService::getReleasedResults(const Session& student) const
{
    auth_.requireRole(student,Role::Student);
    return repo_.listReleasedForStudent(student.userId());
}
} // namespace lms
