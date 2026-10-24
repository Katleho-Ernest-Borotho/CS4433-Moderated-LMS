#include "lms/application/SubmissionManagementService.h"
#include "lms/domain/UtcDateTime.h"
#include <ctime>
#include <iomanip>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

namespace lms {
SubmissionManagementService::SubmissionManagementService(
    const IAuthenticationService& authentication, IAcademicRepository& academic)
    : authentication_(authentication), academic_(academic) {}

SubmissionManagementService::SubmissionManagementService(
    const IAuthenticationService& authentication, IAcademicRepository& academic,
    ISubmissionRepository& submissions, ISubmissionContentStore& files)
    : authentication_(authentication), academic_(academic),
      submissions_(&submissions), files_(&files) {}

namespace {
std::string utcNow()
{
    const std::time_t current=std::time(nullptr);
    std::tm utc{};
#ifdef _WIN32
    gmtime_s(&utc,&current);
#else
    gmtime_r(&current,&utc);
#endif
    std::ostringstream out;
    out << std::put_time(&utc,"%Y-%m-%dT%H:%M:%SZ");
    return out.str();
}
} // namespace

SubmissionVersion SubmissionManagementService::submit(
    const Session& actor, std::int64_t assignmentId, const SubmissionContent& content)
{
    authentication_.requireRole(actor,Role::Student);
    if(!submissions_ || !files_)throw std::runtime_error("Submission storage is not configured");
    const auto assignment=academic_.findAssignmentById(assignmentId);
    if(!assignment)throw std::invalid_argument("Assignment does not exist");
    if(!academic_.isStudentEnrolled(actor.userId(),assignment->courseId))
        throw AuthenticationError("You are not enrolled in this course");

    const bool meaningfulText=content.text.find_first_not_of(" \t\r\n")!=std::string::npos;
    if(!meaningfulText && content.attachmentPath.empty())
        throw std::invalid_argument("Enter submission text or choose an attachment");
    if(content.text.size()>50000)
        throw std::invalid_argument("Submission text exceeds 50,000 characters");

    const std::string submittedAt=utcNow();
    const auto previous=submissions_->findFor(actor.userId(),assignmentId);
    const std::string effective=academic_.findEffectiveDeadline(assignmentId,actor.userId());
    // A first submission is allowed after the deadline. Only replacements must be on time.
    if(previous && std::string_view(submittedAt)>std::string_view(effective))
        throw std::invalid_argument("Resubmission is closed after your effective deadline");

    std::optional<StagedRef> staged;
    std::optional<StoredRef> stored;
    try {
        if(!content.attachmentPath.empty()) {
            staged=files_->stage(content.attachmentPath);
            stored=files_->commit(*staged);
        }
        return submissions_->saveVersion(
            actor.userId(), assignmentId, content.text,
            stored ? std::string_view(stored->token) : std::string_view{},
            submittedAt);
    } catch(...) {
        if(staged) files_->discard(*staged);
        if(stored) files_->remove(*stored);
        throw;
    }
}

std::vector<SubmissionVersion> SubmissionManagementService::listMyVersions(
    const Session& actor, std::int64_t assignmentId) const
{
    authentication_.requireRole(actor,Role::Student);
    if(!submissions_)throw std::runtime_error("Submission storage is not configured");
    const auto assignment=academic_.findAssignmentById(assignmentId);
    if(!assignment)throw std::invalid_argument("Assignment does not exist");
    if(!academic_.isStudentEnrolled(actor.userId(),assignment->courseId))
        throw AuthenticationError("You are not enrolled in this course");
    return submissions_->listVersions(actor.userId(),assignmentId);
}


void SubmissionManagementService::grantExtension(
    const Session& actor, std::int64_t assignmentId, std::int64_t studentId,
    std::string_view newDeadlineUtc)
{
    authentication_.requireRole(actor, Role::Lecturer);
    validateUtcDateTime(newDeadlineUtc);
    const auto assignment=academic_.findAssignmentById(assignmentId);
    if(!assignment) throw std::invalid_argument("Assignment does not exist");
    if(!academic_.isLecturerAssigned(actor.userId(), assignment->courseId))
        throw AuthenticationError("Cannot extend an assignment outside your courses");
    if(!academic_.isStudentEnrolled(studentId, assignment->courseId))
        throw std::invalid_argument("Selected student is not enrolled");
    const std::string currentDeadline=academic_.findEffectiveDeadline(assignmentId, studentId);
    if(newDeadlineUtc <= std::string_view(currentDeadline))
        throw std::invalid_argument("New deadline must be later than the current effective deadline");
    academic_.saveExtension(assignmentId,studentId,actor.userId(),newDeadlineUtc);
}

std::string SubmissionManagementService::effectiveDeadline(
    const Session& actor, std::int64_t assignmentId, std::int64_t studentId) const
{
    const auto assignment=academic_.findAssignmentById(assignmentId);
    if(!assignment) throw std::invalid_argument("Assignment does not exist");
    switch(actor.role()) {
    case Role::Student:
        authentication_.requireRole(actor, Role::Student);
        if(actor.userId()!=studentId ||
           !academic_.isStudentEnrolled(studentId,assignment->courseId))
            throw AuthenticationError("Cannot see another student's deadline");
        break;
    case Role::Lecturer:
        authentication_.requireRole(actor, Role::Lecturer);
        if(!academic_.isLecturerAssigned(actor.userId(),assignment->courseId) ||
           !academic_.isStudentEnrolled(studentId,assignment->courseId))
            throw AuthenticationError("Cannot view this student deadline");
        break;
    case Role::Administrator:
        authentication_.requireRole(actor, Role::Administrator);
        break;
    case Role::HeadOfDepartment:
        authentication_.requireRole(actor, Role::HeadOfDepartment);
        {
            const auto own=academic_.listHoDCourses(actor.userId());
            bool found=false;
            for(const auto& course: own) if(course.id==assignment->courseId) found=true;
            if(!found) throw AuthenticationError("Assignment outside department");
        }
        break;
    }
    return academic_.findEffectiveDeadline(assignmentId,studentId);
}
} // namespace lms
