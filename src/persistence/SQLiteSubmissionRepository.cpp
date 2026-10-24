#include "lms/persistence/SQLiteRecordRepository.h"
#include "lms/domain/UtcDateTime.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace lms {
namespace {
using Statement=std::unique_ptr<sqlite3_stmt,decltype(&sqlite3_finalize)>;

Statement prepare(sqlite3* db,const char* sql)
{
    sqlite3_stmt* raw=nullptr;
    if(sqlite3_prepare_v2(db,sql,-1,&raw,nullptr)!=SQLITE_OK)
        throw std::runtime_error(sqlite3_errmsg(db));
    return Statement(raw,&sqlite3_finalize);
}

void bindId(sqlite3_stmt* s,int index,std::int64_t value)
{
    if(sqlite3_bind_int64(s,index,value)!=SQLITE_OK)
        throw std::runtime_error("Cannot bind database identifier");
}

void bindText(sqlite3_stmt* s,int index,std::string_view value)
{
    if(sqlite3_bind_text(s,index,value.data(),static_cast<int>(value.size()),
                         SQLITE_TRANSIENT)!=SQLITE_OK)
        throw std::runtime_error("Cannot bind database content");
}

std::string columnText(sqlite3_stmt* s,int index)
{
    auto* data=sqlite3_column_text(s,index);
    return data ? reinterpret_cast<const char*>(data) : "";
}

void exec(sqlite3* db,const char* query)
{
    char* error=nullptr;
    if(sqlite3_exec(db,query,nullptr,nullptr,&error)!=SQLITE_OK) {
        const std::string reason=error?error:sqlite3_errmsg(db);
        sqlite3_free(error);
        throw std::runtime_error(reason);
    }
}

class Transaction final {
public:
    explicit Transaction(sqlite3* db):db_(db){exec(db_,"BEGIN IMMEDIATE;");}
    ~Transaction(){if(!committed_)sqlite3_exec(db_,"ROLLBACK;",nullptr,nullptr,nullptr);}
    void commit(){exec(db_,"COMMIT;");committed_=true;}
private:
    sqlite3* db_;
    bool committed_=false;
};

SubmissionVersion versionFromRow(sqlite3_stmt* s)
{
    return SubmissionVersion{
        sqlite3_column_int64(s,0),
        sqlite3_column_int64(s,1),
        sqlite3_column_int(s,2),
        columnText(s,3),
        columnText(s,4),
        columnText(s,5),
        sqlite3_column_int(s,6)==1
    };
}

std::string effectiveDeadline(sqlite3* db,std::int64_t assignment,std::int64_t student)
{
    auto s=prepare(db,
        "SELECT COALESCE(("
        "SELECT de.new_deadline_utc FROM deadline_extensions de "
        "WHERE de.assignment_id=a.assignment_id AND de.student_id=?2"
        "),a.deadline_utc) FROM assignments a WHERE a.assignment_id=?1;");
    bindId(s.get(),1,assignment);
    bindId(s.get(),2,student);
    const int rc=sqlite3_step(s.get());
    if(rc==SQLITE_DONE)throw std::invalid_argument("Assignment does not exist");
    if(rc!=SQLITE_ROW)throw std::runtime_error(sqlite3_errmsg(db));
    return columnText(s.get(),0);
}
} // namespace

std::optional<SubmissionSummary> SQLiteRecordRepository::findFor(
    std::int64_t studentId,std::int64_t assignmentId) const
{
    auto* db=database_.connection();
    auto s=prepare(db,
        "SELECT submission_id,student_id,assignment_id,state FROM submissions "
        "WHERE student_id=?1 AND assignment_id=?2;");
    bindId(s.get(),1,studentId);
    bindId(s.get(),2,assignmentId);
    const int rc=sqlite3_step(s.get());
    if(rc==SQLITE_DONE)return std::nullopt;
    if(rc!=SQLITE_ROW)throw std::runtime_error(sqlite3_errmsg(db));
    return SubmissionSummary{sqlite3_column_int64(s.get(),0),
        sqlite3_column_int64(s.get(),1),sqlite3_column_int64(s.get(),2),
        columnText(s.get(),3)};
}

SubmissionVersion SQLiteRecordRepository::saveVersion(
    std::int64_t studentId,std::int64_t assignmentId,
    std::string_view text,std::string_view attachmentRef,
    std::string_view submittedAtUtc)
{
    validateUtcDateTime(submittedAtUtc);
    if(text.find_first_not_of(" \t\n\r")==std::string_view::npos && attachmentRef.empty())
        throw std::invalid_argument("Enter submission text or attach a supported file");

    auto* db=database_.connection();
    Transaction tx(db);
    auto course=prepare(db,"SELECT course_id FROM assignments WHERE assignment_id=?1;");
    bindId(course.get(),1,assignmentId);
    const int rc=sqlite3_step(course.get());
    if(rc==SQLITE_DONE)throw std::invalid_argument("Assignment does not exist");
    if(rc!=SQLITE_ROW)throw std::runtime_error(sqlite3_errmsg(db));
    const auto courseId=sqlite3_column_int64(course.get(),0);
    course.reset();

    auto enrolled=prepare(db,
        "SELECT EXISTS(SELECT 1 FROM enrollments e "
        "JOIN users u ON u.user_id=e.student_id "
        "WHERE e.student_id=?1 AND e.course_id=?2 AND u.active=1 AND u.role='Student');");
    bindId(enrolled.get(),1,studentId);
    bindId(enrolled.get(),2,courseId);
    if(sqlite3_step(enrolled.get())!=SQLITE_ROW ||
       sqlite3_column_int(enrolled.get(),0)!=1)
        throw std::invalid_argument("Student is not actively enrolled in this course");
    enrolled.reset();

    const std::string deadline=effectiveDeadline(db,assignmentId,studentId);
    const bool late=submittedAtUtc>std::string_view(deadline);
    auto existing=prepare(db,
        "SELECT submission_id FROM submissions WHERE student_id=?1 AND assignment_id=?2;");
    bindId(existing.get(),1,studentId);
    bindId(existing.get(),2,assignmentId);
    const int found=sqlite3_step(existing.get());
    if(found!=SQLITE_ROW && found!=SQLITE_DONE)
        throw std::runtime_error(sqlite3_errmsg(db));
    const bool isReplacement=found==SQLITE_ROW;
    const auto oldId=isReplacement?sqlite3_column_int64(existing.get(),0):0;
    existing.reset();

    // First submissions are accepted even when late; replacements are deadline-limited.
    if(isReplacement && late)
        throw std::invalid_argument("The effective deadline has passed; resubmission is closed");
    const char* state=late?"Late":"Submitted";
    std::int64_t submissionId=oldId;
    if(!isReplacement) {
        auto insert=prepare(db,
            "INSERT INTO submissions(student_id,assignment_id,state) VALUES(?1,?2,?3);");
        bindId(insert.get(),1,studentId);
        bindId(insert.get(),2,assignmentId);
        bindText(insert.get(),3,state);
        if(sqlite3_step(insert.get())!=SQLITE_DONE)
            throw std::runtime_error(sqlite3_errmsg(db));
        submissionId=sqlite3_last_insert_rowid(db);
    } else {
        auto update=prepare(db,
            "UPDATE submissions SET state=?1 WHERE submission_id=?2;");
        bindText(update.get(),1,state);
        bindId(update.get(),2,submissionId);
        if(sqlite3_step(update.get())!=SQLITE_DONE)
            throw std::runtime_error(sqlite3_errmsg(db));
    }

    auto next=prepare(db,
        "SELECT COALESCE(MAX(version_number),0)+1 FROM submission_versions "
        "WHERE submission_id=?1;");
    bindId(next.get(),1,submissionId);
    if(sqlite3_step(next.get())!=SQLITE_ROW)
        throw std::runtime_error(sqlite3_errmsg(db));
    const int number=sqlite3_column_int(next.get(),0);
    next.reset();

    auto insertVersion=prepare(db,
        "INSERT INTO submission_versions("
        "submission_id,version_number,submitted_at_utc,text_content,attachment_ref,is_late"
        ") VALUES(?1,?2,?3,?4,?5,?6);");
    bindId(insertVersion.get(),1,submissionId);
    if(sqlite3_bind_int(insertVersion.get(),2,number)!=SQLITE_OK)
        throw std::runtime_error("Cannot bind version number");
    bindText(insertVersion.get(),3,submittedAtUtc);
    if(text.empty()) {
        if(sqlite3_bind_null(insertVersion.get(),4)!=SQLITE_OK)
            throw std::runtime_error("Cannot bind empty submission text");
    } else bindText(insertVersion.get(),4,text);
    if(attachmentRef.empty()) {
        if(sqlite3_bind_null(insertVersion.get(),5)!=SQLITE_OK)
            throw std::runtime_error("Cannot bind empty attachment");
    } else bindText(insertVersion.get(),5,attachmentRef);
    if(sqlite3_bind_int(insertVersion.get(),6,late?1:0)!=SQLITE_OK)
        throw std::runtime_error("Cannot bind late status");
    if(sqlite3_step(insertVersion.get())!=SQLITE_DONE)
        throw std::runtime_error(sqlite3_errmsg(db));
    const auto versionId=sqlite3_last_insert_rowid(db);
    insertVersion.reset();
    tx.commit();
    return SubmissionVersion{versionId,submissionId,number,
        std::string(submittedAtUtc),std::string(text),std::string(attachmentRef),late};
}

std::vector<SubmissionVersion> SQLiteRecordRepository::listVersions(
    std::int64_t studentId,std::int64_t assignmentId) const
{
    auto* db=database_.connection();
    auto s=prepare(db,
        "SELECT v.version_id,v.submission_id,v.version_number,v.submitted_at_utc,"
        "v.text_content,v.attachment_ref,v.is_late "
        "FROM submission_versions v "
        "JOIN submissions sub ON sub.submission_id=v.submission_id "
        "WHERE sub.student_id=?1 AND sub.assignment_id=?2 "
        "ORDER BY v.version_number DESC;");
    bindId(s.get(),1,studentId);
    bindId(s.get(),2,assignmentId);
    std::vector<SubmissionVersion> result;
    for(;;) {
        const int rc=sqlite3_step(s.get());
        if(rc==SQLITE_DONE)return result;
        if(rc!=SQLITE_ROW)throw std::runtime_error(sqlite3_errmsg(db));
        result.push_back(versionFromRow(s.get()));
    }
}
} // namespace lms
