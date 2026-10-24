#include "lms/persistence/SQLiteRecordRepository.h"
#include "lms/domain/UtcDateTime.h"
#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace lms {
namespace {
using Statement=std::unique_ptr<sqlite3_stmt,decltype(&sqlite3_finalize)>;

Statement prepare(sqlite3* db,const char* query)
{
    sqlite3_stmt* ptr=nullptr;
    if(sqlite3_prepare_v2(db,query,-1,&ptr,nullptr)!=SQLITE_OK)
        throw std::runtime_error(sqlite3_errmsg(db));
    return Statement(ptr,&sqlite3_finalize);
}
void bindId(sqlite3_stmt* s,int n,std::int64_t id)
{
    if(sqlite3_bind_int64(s,n,id)!=SQLITE_OK)
        throw std::runtime_error("Failed to bind database identifier");
}
void bindText(sqlite3_stmt* s,int n,std::string_view text)
{
    if(sqlite3_bind_text(s,n,text.data(),static_cast<int>(text.size()),
                         SQLITE_TRANSIENT)!=SQLITE_OK)
        throw std::runtime_error("Failed to bind database text");
}
void bindDouble(sqlite3_stmt* s,int n,double value)
{
    if(sqlite3_bind_double(s,n,value)!=SQLITE_OK)
        throw std::runtime_error("Failed to bind mark");
}
std::string getText(sqlite3_stmt* s,int n)
{
    const auto* text=sqlite3_column_text(s,n);
    return text?reinterpret_cast<const char*>(text):"";
}
void exec(sqlite3* db,const char* sql)
{
    char* err=nullptr;
    if(sqlite3_exec(db,sql,nullptr,nullptr,&err)!=SQLITE_OK) {
        const std::string reason=err?err:sqlite3_errmsg(db);
        sqlite3_free(err);
        throw std::runtime_error(reason);
    }
}
class Transaction {
public:
    explicit Transaction(sqlite3* db):db_(db){exec(db_,"BEGIN IMMEDIATE;");}
    ~Transaction(){if(!complete_)sqlite3_exec(db_,"ROLLBACK;",nullptr,nullptr,nullptr);}
    void commit(){exec(db_,"COMMIT;");complete_=true;}
private:
    sqlite3* db_;
    bool complete_=false;
};

const char* moderationQuery=
    "SELECT r.result_id,s.submission_id,s.assignment_id,s.student_id,"
    "r.marked_version_id,v.version_id,u.username,c.code,a.title,"
    "v.submitted_at_utc,COALESCE(de.new_deadline_utc,a.deadline_utc),r.feedback,"
    "a.maximum_mark,r.earned_mark,r.penalty_mark,r.final_mark,r.approved,"
    "COALESCE(r.released_at_utc,'') "
    "FROM assessment_results r "
    "JOIN submissions s ON s.submission_id=r.submission_id "
    "JOIN users u ON u.user_id=s.student_id "
    "JOIN assignments a ON a.assignment_id=s.assignment_id "
    "JOIN courses c ON c.course_id=a.course_id "
    "JOIN hod_departments h ON h.department_id=c.department_id AND h.hod_id=?1 "
    "JOIN users hod ON hod.user_id=h.hod_id AND hod.active=1 AND hod.role='HeadOfDepartment' "
    "JOIN submission_versions v ON v.submission_id=s.submission_id "
    "AND v.version_number=(SELECT MAX(v2.version_number) FROM submission_versions v2 "
    "WHERE v2.submission_id=s.submission_id) "
    "LEFT JOIN deadline_extensions de ON de.assignment_id=s.assignment_id "
    "AND de.student_id=s.student_id "
    "WHERE r.released_at_utc IS NULL ";

ModerationRecord readModeration(sqlite3_stmt* s)
{
    return ModerationRecord{
        sqlite3_column_int64(s,0),sqlite3_column_int64(s,1),
        sqlite3_column_int64(s,2),sqlite3_column_int64(s,3),
        sqlite3_column_int64(s,4),sqlite3_column_int64(s,5),
        getText(s,6),getText(s,7),getText(s,8),getText(s,9),getText(s,10),
        getText(s,11),sqlite3_column_double(s,12),sqlite3_column_double(s,13),
        sqlite3_column_double(s,14),sqlite3_column_double(s,15),
        sqlite3_column_int(s,16)==1,getText(s,17)};
}

ModerationRecord currentTarget(sqlite3* db,std::int64_t hod,std::int64_t submission)
{
    const std::string query=std::string(moderationQuery)+"AND s.submission_id=?2;";
    auto s=prepare(db,query.c_str());
    bindId(s.get(),1,hod);
    bindId(s.get(),2,submission);
    if(sqlite3_step(s.get())!=SQLITE_ROW)
        throw std::invalid_argument("Unreleased result not found in this department");
    return readModeration(s.get());
}

void audit(sqlite3* db,std::int64_t result,std::int64_t hod,
    std::string_view timestamp,const char* field,double oldValue,double newValue,
    std::string_view reason)
{
    if(oldValue==newValue)return;
    auto statement=prepare(db,
        "INSERT INTO mark_change_audits(result_id,changed_by_user_id,changed_at_utc,"
        "changed_field,old_mark,new_mark,reason) VALUES(?1,?2,?3,?4,?5,?6,?7);");
    bindId(statement.get(),1,result);
    bindId(statement.get(),2,hod);
    bindText(statement.get(),3,timestamp);
    bindText(statement.get(),4,field);
    bindDouble(statement.get(),5,oldValue);
    bindDouble(statement.get(),6,newValue);
    bindText(statement.get(),7,reason);
    if(sqlite3_step(statement.get())!=SQLITE_DONE)
        throw std::runtime_error(sqlite3_errmsg(db));
}
void guardTimeVersion(const ModerationRecord& current,std::int64_t expectedVersionId,
                      std::string_view nowUtc)
{
    if(current.markedVersionId!=current.latestVersionId ||
       current.latestVersionId!=expectedVersionId)
        throw std::invalid_argument("Assessment has a newer, unmarked submission version");
    if(nowUtc<=std::string_view(current.effectiveDeadlineUtc))
        throw std::invalid_argument("Moderation/release must occur after the effective deadline");
}
}

std::vector<ModerationRecord> SQLiteRecordRepository::listDepartmentResults(
    std::int64_t hodId) const
{
    auto* db=database_.connection();
    const std::string query=std::string(moderationQuery)+
        "ORDER BY c.code COLLATE NOCASE,u.username COLLATE NOCASE;";
    auto s=prepare(db,query.c_str());
    bindId(s.get(),1,hodId);
    std::vector<ModerationRecord> records;
    for(;;) {
        const int rc=sqlite3_step(s.get());
        if(rc==SQLITE_DONE)return records;
        if(rc!=SQLITE_ROW)throw std::runtime_error(sqlite3_errmsg(db));
        records.push_back(readModeration(s.get()));
    }
}

void SQLiteRecordRepository::approveMark(
    std::int64_t hodId,std::int64_t submissionId,std::int64_t expectedVersionId,
    double earned,double penalty,double finalMark,std::string_view reason,
    std::string_view nowUtc)
{
    validateUtcDateTime(nowUtc);
    if(!std::isfinite(earned)||!std::isfinite(penalty)||!std::isfinite(finalMark))
        throw std::invalid_argument("Marks must be finite");
    auto* db=database_.connection();
    Transaction tx(db);
    const auto record=currentTarget(db,hodId,submissionId);
    guardTimeVersion(record,expectedVersionId,nowUtc);
    if(record.approved)throw std::invalid_argument("Result has already been approved");
    if(earned<0.0||earned>record.maximumMark||penalty<0.0||
       finalMark<0.0||finalMark>record.maximumMark)
        throw std::invalid_argument("Adjusted mark is out of bounds");
    const bool changed=record.earnedMark!=earned || record.penaltyMark!=penalty ||
                       record.finalMark!=finalMark;
    if(changed && (reason.empty() ||
       reason.find_first_not_of(" \t\r\n")==std::string_view::npos))
        throw std::invalid_argument("Moderation adjustment needs an audit reason");

    auto update=prepare(db,
        "UPDATE assessment_results SET earned_mark=?1,penalty_mark=?2,"
        "final_mark=?3,approved=1,moderated_by_hod_id=?4,moderated_at_utc=?5 "
        "WHERE result_id=?6 AND approved=0 AND released_at_utc IS NULL;");
    bindDouble(update.get(),1,earned);
    bindDouble(update.get(),2,penalty);
    bindDouble(update.get(),3,finalMark);
    bindId(update.get(),4,hodId);
    bindText(update.get(),5,nowUtc);
    bindId(update.get(),6,record.resultId);
    if(sqlite3_step(update.get())!=SQLITE_DONE)
        throw std::runtime_error(sqlite3_errmsg(db));
    if(sqlite3_changes(db)!=1)
        throw std::runtime_error("Result changed before approval");
    update.reset();

    audit(db,record.resultId,hodId,nowUtc,"earned_mark",
          record.earnedMark,earned,reason);
    audit(db,record.resultId,hodId,nowUtc,"penalty_mark",
          record.penaltyMark,penalty,reason);
    audit(db,record.resultId,hodId,nowUtc,"final_mark",
          record.finalMark,finalMark,reason);

    auto state=prepare(db,"UPDATE submissions SET state='Moderated' WHERE submission_id=?1;");
    bindId(state.get(),1,submissionId);
    if(sqlite3_step(state.get())!=SQLITE_DONE)
        throw std::runtime_error(sqlite3_errmsg(db));
    state.reset();
    tx.commit();
}

void SQLiteRecordRepository::releaseMark(
    std::int64_t hodId,std::int64_t submissionId,std::int64_t expectedVersionId,
    std::string_view nowUtc)
{
    validateUtcDateTime(nowUtc);
    auto* db=database_.connection();
    Transaction tx(db);
    const auto record=currentTarget(db,hodId,submissionId);
    guardTimeVersion(record,expectedVersionId,nowUtc);
    if(!record.approved)
        throw std::invalid_argument("Only moderated, approved results can be released");
    auto state=prepare(db,"SELECT state FROM submissions WHERE submission_id=?1;");
    bindId(state.get(),1,submissionId);
    if(sqlite3_step(state.get())!=SQLITE_ROW || getText(state.get(),0)!="Moderated")
        throw std::invalid_argument("Submission is not ready for release");
    state.reset();

    auto release=prepare(db,
        "UPDATE assessment_results SET released_at_utc=?1 "
        "WHERE result_id=?2 AND approved=1 AND released_at_utc IS NULL;");
    bindText(release.get(),1,nowUtc);
    bindId(release.get(),2,record.resultId);
    if(sqlite3_step(release.get())!=SQLITE_DONE)
        throw std::runtime_error(sqlite3_errmsg(db));
    if(sqlite3_changes(db)!=1)
        throw std::runtime_error("Result was already released or changed");
    release.reset();
    auto update=prepare(db,"UPDATE submissions SET state='Released' WHERE submission_id=?1;");
    bindId(update.get(),1,submissionId);
    if(sqlite3_step(update.get())!=SQLITE_DONE)
        throw std::runtime_error(sqlite3_errmsg(db));
    update.reset();
    tx.commit();
}

std::vector<ReleasedResult> SQLiteRecordRepository::listReleasedForStudent(
    std::int64_t studentId) const
{
    auto* db=database_.connection();
    auto s=prepare(db,
        "SELECT s.submission_id,a.assignment_id,c.code,a.title,r.feedback,"
        "r.released_at_utc,a.maximum_mark,r.earned_mark,r.penalty_mark,r.final_mark "
        "FROM assessment_results r JOIN submissions s ON s.submission_id=r.submission_id "
        "JOIN assignments a ON a.assignment_id=s.assignment_id "
        "JOIN courses c ON c.course_id=a.course_id "
        "WHERE s.student_id=?1 AND r.approved=1 AND r.released_at_utc IS NOT NULL "
        "AND s.state='Released' AND r.marked_version_id=("
        "SELECT v.version_id FROM submission_versions v WHERE v.submission_id=s.submission_id "
        "ORDER BY v.version_number DESC LIMIT 1) "
        "ORDER BY r.released_at_utc DESC,s.submission_id DESC;");
    bindId(s.get(),1,studentId);
    std::vector<ReleasedResult> out;
    for(;;) {
        const int rc=sqlite3_step(s.get());
        if(rc==SQLITE_DONE)return out;
        if(rc!=SQLITE_ROW)throw std::runtime_error(sqlite3_errmsg(db));
        out.push_back(ReleasedResult{
            sqlite3_column_int64(s.get(),0),sqlite3_column_int64(s.get(),1),
            getText(s.get(),2),getText(s.get(),3),getText(s.get(),4),getText(s.get(),5),
            sqlite3_column_double(s.get(),6),sqlite3_column_double(s.get(),7),
            sqlite3_column_double(s.get(),8),sqlite3_column_double(s.get(),9)});
    }
}
} // namespace lms
