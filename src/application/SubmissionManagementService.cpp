#include "lms/application/SubmissionManagementService.h"
#include "lms/domain/UtcDateTime.h"
#include <stdexcept>
#include <string>

namespace lms {
SubmissionManagementService::SubmissionManagementService(
    const IAuthenticationService& authentication, IAcademicRepository& academic)
    : authentication_(authentication), academic_(academic) {}

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
