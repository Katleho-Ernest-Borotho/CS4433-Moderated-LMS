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

Statement sql(sqlite3* db,const char* query) {
    sqlite3_stmt* ptr=nullptr;
    if(sqlite3_prepare_v2(db,query,-1,&ptr,nullptr)!=SQLITE_OK)
        throw std::runtime_error(sqlite3_errmsg(db));
    return Statement(ptr,&sqlite3_finalize);
}
void id(sqlite3_stmt* s,int n,std::int64_t value) {
    if(sqlite3_bind_int64(s,n,value)!=SQLITE_OK)
        throw std::runtime_error("Cannot bind identifier");
}
void txt(sqlite3_stmt* s,int n,std::string_view value) {
    if(sqlite3_bind_text(s,n,value.data(),static_cast<int>(value.size()),
                         SQLITE_TRANSIENT)!=SQLITE_OK)
        throw std::runtime_error("Cannot bind text");
}
void dbl(sqlite3_stmt* s,int n,double value) {
    if(sqlite3_bind_double(s,n,value)!=SQLITE_OK)
        throw std::runtime_error("Cannot bind mark");
}
std::string str(sqlite3_stmt* s,int n) {
    const auto* value=sqlite3_column_text(s,n);
    return value?reinterpret_cast<const char*>(value):"";
}
void exec(sqlite3* db,const char* command) {
    char* error=nullptr;
    if(sqlite3_exec(db,command,nullptr,nullptr,&error)!=SQLITE_OK) {
        const std::string msg=error?error:sqlite3_errmsg(db);
        sqlite3_free(error);
        throw std::runtime_error(msg);
    }
}
class Transaction {
public:
    explicit Transaction(sqlite3* db):db_(db){exec(db_,"BEGIN IMMEDIATE;");}
    ~Transaction(){if(!committed_)sqlite3_exec(db_,"ROLLBACK;",nullptr,nullptr,nullptr);}
    void commit(){exec(db_,"COMMIT;");committed_=true;}
private:
    sqlite3* db_;
    bool committed_=false;
};
MarkableSubmission makeSubmission(sqlite3_stmt* s) {
    return MarkableSubmission{
        sqlite3_column_int64(s,0),sqlite3_column_int64(s,1),
        sqlite3_column_int64(s,2),sqlite3_column_int64(s,3),
        sqlite3_column_int(s,4),str(s,5),str(s,6),
        str(s,7),str(s,8),str(s,9)};
}
const char* submissionQuery =
    "SELECT s.submission_id,s.student_id,s.assignment_id,"
    "v.version_id,v.version_number,u.username,v.submitted_at_utc,"
    "COALESCE(v.text_content,''),COALESCE(v.attachment_ref,''),s.state "
    "FROM submissions s JOIN users u ON u.user_id=s.student_id "
    "JOIN submission_versions v ON v.submission_id=s.submission_id "
    "AND v.version_number=("
    "SELECT MAX(v2.version_number) FROM submission_versions v2 "
    "WHERE v2.submission_id=s.submission_id) ";
AssessmentResult makeResult(sqlite3_stmt* s) {
    return AssessmentResult{
        sqlite3_column_int64(s,0),sqlite3_column_int64(s,1),
        sqlite3_column_int64(s,2),sqlite3_column_int64(s,3),
        sqlite3_column_double(s,4),sqlite3_column_double(s,5),
        sqlite3_column_double(s,6),str(s,7),
        sqlite3_column_int(s,8)==1,str(s,9)};
}
const char* resultQuery =
    "SELECT result_id,submission_id,marked_version_id,marked_by_lecturer_id,"
    "earned_mark,penalty_mark,final_mark,feedback,approved,"
    "COALESCE(released_at_utc,'') FROM assessment_results WHERE submission_id=?1;";
std::optional<AssessmentResult> getResult(sqlite3* db,std::int64_t submission) {
    auto s=sql(db,resultQuery);
    id(s.get(),1,submission);
    const int rc=sqlite3_step(s.get());
    if(rc==SQLITE_DONE)return std::nullopt;
    if(rc!=SQLITE_ROW)throw std::runtime_error(sqlite3_errmsg(db));
    return makeResult(s.get());
}
} // namespace

std::vector<MarkableSubmission> SQLiteRecordRepository::listAssignmentSubmissions(
    std::int64_t assignmentId) const
{
    auto* db=database_.connection();
    const std::string query=std::string(submissionQuery)+
        "WHERE s.assignment_id=?1 ORDER BY u.username COLLATE NOCASE;";
    auto s=sql(db,query.c_str());
    id(s.get(),1,assignmentId);
    std::vector<MarkableSubmission> result;
    for(;;) {
        const int rc=sqlite3_step(s.get());
        if(rc==SQLITE_DONE)return result;
        if(rc!=SQLITE_ROW)throw std::runtime_error(sqlite3_errmsg(db));
        result.push_back(makeSubmission(s.get()));
    }
}

std::optional<MarkableSubmission> SQLiteRecordRepository::findMarkableSubmission(
    std::int64_t submissionId) const
{
    auto* db=database_.connection();
    const std::string query=std::string(submissionQuery)+"WHERE s.submission_id=?1;";
    auto s=sql(db,query.c_str());
    id(s.get(),1,submissionId);
    const int rc=sqlite3_step(s.get());
    if(rc==SQLITE_DONE)return std::nullopt;
    if(rc!=SQLITE_ROW)throw std::runtime_error(sqlite3_errmsg(db));
    return makeSubmission(s.get());
}

std::optional<AssessmentResult> SQLiteRecordRepository::findResult(
    std::int64_t submissionId) const
{
    return getResult(database_.connection(),submissionId);
}

AssessmentResult SQLiteRecordRepository::saveUnreleasedMark(
    std::int64_t submissionId,std::int64_t expectedVersionId,std::int64_t lecturerId,
    double earned,double penalty,double finalMark,std::string_view feedback,
    std::string_view revisionReason,std::string_view changedAt)
{
    validateUtcDateTime(changedAt);
    auto* db=database_.connection();
    Transaction tx(db);
    const auto found=findMarkableSubmission(submissionId);
    if(!found)throw std::invalid_argument("Submission not found");
    if(found->latestVersionId!=expectedVersionId)
        throw std::runtime_error("Submission was revised; reload and mark the latest version");
    if(found->state=="Moderated"||found->state=="Released")
        throw std::invalid_argument("Approved or released results cannot be edited here");
    auto old=getResult(db,submissionId);
    if(old && (old->approved||!old->releasedAtUtc.empty()))
        throw std::invalid_argument("An approved/released mark cannot be edited");
    const bool changes=old && (old->earnedMark!=earned||
        old->penaltyMark!=penalty||old->finalMark!=finalMark);
    if(old && (changes || old->markedVersionId!=expectedVersionId ||
               old->feedback!=feedback) &&
       revisionReason.find_first_not_of(" \t\r\n")==std::string_view::npos)
        throw std::invalid_argument("Give a reason for revising an existing result");

    std::int64_t resultId=0;
    if(!old) {
        auto insert=sql(db,
            "INSERT INTO assessment_results("
            "submission_id,marked_version_id,marked_by_lecturer_id,"
            "earned_mark,penalty_mark,final_mark,feedback,approved"
            ") VALUES(?1,?2,?3,?4,?5,?6,?7,0);");
        id(insert.get(),1,submissionId);
        id(insert.get(),2,expectedVersionId);
        id(insert.get(),3,lecturerId);
        dbl(insert.get(),4,earned);
        dbl(insert.get(),5,penalty);
        dbl(insert.get(),6,finalMark);
        txt(insert.get(),7,feedback);
        if(sqlite3_step(insert.get())!=SQLITE_DONE)
            throw std::runtime_error(sqlite3_errmsg(db));
        resultId=sqlite3_last_insert_rowid(db);
    } else {
        resultId=old->resultId;
        auto update=sql(db,
            "UPDATE assessment_results SET marked_version_id=?1,"
            "marked_by_lecturer_id=?2,earned_mark=?3,penalty_mark=?4,"
            "final_mark=?5,feedback=?6,approved=0,"
            "moderated_by_hod_id=NULL,moderated_at_utc=NULL,released_at_utc=NULL "
            "WHERE result_id=?7;");
        id(update.get(),1,expectedVersionId);
        id(update.get(),2,lecturerId);
        dbl(update.get(),3,earned);
        dbl(update.get(),4,penalty);
        dbl(update.get(),5,finalMark);
        txt(update.get(),6,feedback);
        id(update.get(),7,resultId);
        if(sqlite3_step(update.get())!=SQLITE_DONE)
            throw std::runtime_error(sqlite3_errmsg(db));

        auto audit=[&](const char* field,double before,double after) {
            if(before==after)return;
            auto s=sql(db,
                "INSERT INTO mark_change_audits("
                "result_id,changed_by_user_id,changed_at_utc,changed_field,"
                "old_mark,new_mark,reason) VALUES(?1,?2,?3,?4,?5,?6,?7);");
            id(s.get(),1,resultId);
            id(s.get(),2,lecturerId);
            txt(s.get(),3,changedAt);
            txt(s.get(),4,field);
            dbl(s.get(),5,before);
            dbl(s.get(),6,after);
            txt(s.get(),7,revisionReason);
            if(sqlite3_step(s.get())!=SQLITE_DONE)
                throw std::runtime_error(sqlite3_errmsg(db));
        };
        audit("earned_mark",old->earnedMark,earned);
        audit("penalty_mark",old->penaltyMark,penalty);
        audit("final_mark",old->finalMark,finalMark);
    }
    auto status=sql(db,"UPDATE submissions SET state='Marked' WHERE submission_id=?1;");
    id(status.get(),1,submissionId);
    if(sqlite3_step(status.get())!=SQLITE_DONE)
        throw std::runtime_error(sqlite3_errmsg(db));
    status.reset();
    tx.commit();
    const auto saved=getResult(db,submissionId);
    if(!saved)throw std::runtime_error("Saved mark not found");
    return *saved;
}

std::vector<MarkChangeAudit> SQLiteRecordRepository::listMarkAudits(
    std::int64_t resultId) const
{
    auto* db=database_.connection();
    auto s=sql(db,
        "SELECT result_id,changed_by_user_id,changed_field,old_mark,new_mark,reason "
        "FROM mark_change_audits WHERE result_id=?1 ORDER BY audit_id;");
    id(s.get(),1,resultId);
    std::vector<MarkChangeAudit> rows;
    for(;;) {
        const int rc=sqlite3_step(s.get());
        if(rc==SQLITE_DONE)return rows;
        if(rc!=SQLITE_ROW)throw std::runtime_error(sqlite3_errmsg(db));
        rows.push_back(MarkChangeAudit{
            sqlite3_column_int64(s.get(),0),sqlite3_column_int64(s.get(),1),
            str(s.get(),2),sqlite3_column_double(s.get(),3),
            sqlite3_column_double(s.get(),4),str(s.get(),5)});
    }
}
} // namespace lms
