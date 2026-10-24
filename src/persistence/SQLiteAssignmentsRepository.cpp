#include "lms/persistence/SQLiteRecordRepository.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace lms {
namespace {
using Statement = std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;

Statement prepare(sqlite3* db, const char* query)
{
    sqlite3_stmt* raw=nullptr;
    if(sqlite3_prepare_v2(db,query,-1,&raw,nullptr)!=SQLITE_OK)
        throw std::runtime_error(sqlite3_errmsg(db));
    return Statement(raw,&sqlite3_finalize);
}

void bindId(sqlite3_stmt* s,int position,std::int64_t id)
{
    if(sqlite3_bind_int64(s,position,id)!=SQLITE_OK)
        throw std::runtime_error("Cannot bind identifier");
}

void bindText(sqlite3_stmt* s,int position,std::string_view value)
{
    if(sqlite3_bind_text(s,position,value.data(),static_cast<int>(value.size()),
                         SQLITE_TRANSIENT)!=SQLITE_OK)
        throw std::runtime_error("Cannot bind text");
}

std::string columnText(sqlite3_stmt* s,int n)
{
    auto* raw=sqlite3_column_text(s,n);
    return raw?reinterpret_cast<const char*>(raw):"";
}

void exec(sqlite3* db,const char* query)
{
    char* error=nullptr;
    if(sqlite3_exec(db,query,nullptr,nullptr,&error)!=SQLITE_OK) {
        const std::string message=error?error:sqlite3_errmsg(db);
        sqlite3_free(error);
        throw std::runtime_error(message);
    }
}

class Transaction final {
public:
    explicit Transaction(sqlite3* db):db_(db){exec(db_,"BEGIN IMMEDIATE;");}
    ~Transaction(){if(!committed_)sqlite3_exec(db_,"ROLLBACK;",nullptr,nullptr,nullptr);}
    void commit(){exec(db_,"COMMIT;");committed_=true;}
    Transaction(const Transaction&)=delete;
    Transaction& operator=(const Transaction&)=delete;
private:
    sqlite3* db_;
    bool committed_=false;
};

bool relation(sqlite3* db,const char* query,std::int64_t first,std::int64_t second)
{
    auto s=prepare(db,query);
    bindId(s.get(),1,first);
    bindId(s.get(),2,second);
    if(sqlite3_step(s.get())!=SQLITE_ROW)throw std::runtime_error(sqlite3_errmsg(db));
    return sqlite3_column_int(s.get(),0)==1;
}

bool lecturerAssigned(sqlite3* db,std::int64_t lecturer,std::int64_t course)
{
    return relation(db,
        "SELECT EXISTS(SELECT 1 FROM teaching_assignments t "
        "JOIN users u ON u.user_id=t.lecturer_id "
        "WHERE t.lecturer_id=?1 AND t.course_id=?2 "
        "AND u.active=1 AND u.role='Lecturer');",lecturer,course);
}

bool studentEnrolled(sqlite3* db,std::int64_t student,std::int64_t course)
{
    return relation(db,
        "SELECT EXISTS(SELECT 1 FROM enrollments e "
        "JOIN users u ON u.user_id=e.student_id "
        "WHERE e.student_id=?1 AND e.course_id=?2 "
        "AND u.active=1 AND u.role='Student');",student,course);
}

Assignment mapAssignment(sqlite3_stmt* s)
{
    return Assignment{sqlite3_column_int64(s,0),sqlite3_column_int64(s,1),
                      columnText(s,2),columnText(s,3),sqlite3_column_double(s,4)};
}

std::string effective(sqlite3* db,std::int64_t assignment,std::int64_t student)
{
    auto s=prepare(db,
        "SELECT COALESCE(("
        " SELECT de.new_deadline_utc FROM deadline_extensions de "
        " WHERE de.assignment_id=a.assignment_id AND de.student_id=?2"
        "),a.deadline_utc) FROM assignments a WHERE a.assignment_id=?1;");
    bindId(s.get(),1,assignment);
    bindId(s.get(),2,student);
    const int result=sqlite3_step(s.get());
    if(result==SQLITE_DONE)throw std::invalid_argument("Assignment not found");
    if(result!=SQLITE_ROW)throw std::runtime_error(sqlite3_errmsg(db));
    return columnText(s.get(),0);
}
} // namespace

bool SQLiteRecordRepository::isLecturerAssigned(std::int64_t lecturer,
                                                  std::int64_t course) const
{
    return lecturerAssigned(database_.connection(),lecturer,course);
}

bool SQLiteRecordRepository::isStudentEnrolled(std::int64_t student,
                                                 std::int64_t course) const
{
    return studentEnrolled(database_.connection(),student,course);
}

std::vector<UserAccount> SQLiteRecordRepository::listEnrolledStudents(
    std::int64_t courseId) const
{
    auto* db=database_.connection();
    auto s=prepare(db,
        "SELECT u.user_id,u.username,u.active FROM users u "
        "JOIN enrollments e ON e.student_id=u.user_id "
        "WHERE e.course_id=?1 AND u.role='Student' AND u.active=1 "
        "ORDER BY u.username COLLATE NOCASE;");
    bindId(s.get(),1,courseId);
    std::vector<UserAccount> users;
    for(;;) {
        const int result=sqlite3_step(s.get());
        if(result==SQLITE_DONE)return users;
        if(result!=SQLITE_ROW)throw std::runtime_error(sqlite3_errmsg(db));
        users.push_back(UserAccount{sqlite3_column_int64(s.get(),0),
            columnText(s.get(),1),Role::Student,sqlite3_column_int(s.get(),2)==1});
    }
}

Assignment SQLiteRecordRepository::saveAssignment(
    std::int64_t course,std::string_view title,std::string_view deadline,double maximum)
{
    auto* db=database_.connection();
    Transaction tx(db);
    auto exists=prepare(db,"SELECT EXISTS(SELECT 1 FROM courses WHERE course_id=?1);");
    bindId(exists.get(),1,course);
    if(sqlite3_step(exists.get())!=SQLITE_ROW || sqlite3_column_int(exists.get(),0)!=1)
        throw std::invalid_argument("Course not found");
    exists.reset();
    auto s=prepare(db,
        "INSERT INTO assignments(course_id,title,deadline_utc,maximum_mark) "
        "VALUES(?1,?2,?3,?4);");
    bindId(s.get(),1,course);
    bindText(s.get(),2,title);
    bindText(s.get(),3,deadline);
    if(sqlite3_bind_double(s.get(),4,maximum)!=SQLITE_OK)
        throw std::runtime_error("Cannot bind maximum mark");
    if(sqlite3_step(s.get())!=SQLITE_DONE)throw std::runtime_error(sqlite3_errmsg(db));
    const auto id=sqlite3_last_insert_rowid(db);
    s.reset();
    tx.commit();
    return Assignment{id,course,std::string(title),std::string(deadline),maximum};
}

std::optional<Assignment> SQLiteRecordRepository::findAssignmentById(
    std::int64_t assignment) const
{
    auto* db=database_.connection();
    auto s=prepare(db,
        "SELECT assignment_id,course_id,title,deadline_utc,maximum_mark "
        "FROM assignments WHERE assignment_id=?1;");
    bindId(s.get(),1,assignment);
    const int step=sqlite3_step(s.get());
    if(step==SQLITE_DONE)return std::nullopt;
    if(step!=SQLITE_ROW)throw std::runtime_error(sqlite3_errmsg(db));
    return mapAssignment(s.get());
}

std::vector<Assignment> SQLiteRecordRepository::listCourseAssignments(
    std::int64_t course) const
{
    auto* db=database_.connection();
    auto s=prepare(db,
        "SELECT assignment_id,course_id,title,deadline_utc,maximum_mark "
        "FROM assignments WHERE course_id=?1 ORDER BY deadline_utc,assignment_id;");
    bindId(s.get(),1,course);
    std::vector<Assignment> assignments;
    for(;;) {
        const int step=sqlite3_step(s.get());
        if(step==SQLITE_DONE)return assignments;
        if(step!=SQLITE_ROW)throw std::runtime_error(sqlite3_errmsg(db));
        assignments.push_back(mapAssignment(s.get()));
    }
}

void SQLiteRecordRepository::saveExtension(
    std::int64_t assignment,std::int64_t student,std::int64_t lecturer,
    std::string_view deadline)
{
    auto* db=database_.connection();
    Transaction tx(db);
    auto a=prepare(db,"SELECT course_id FROM assignments WHERE assignment_id=?1;");
    bindId(a.get(),1,assignment);
    const int step=sqlite3_step(a.get());
    if(step==SQLITE_DONE)throw std::invalid_argument("Assignment not found");
    if(step!=SQLITE_ROW)throw std::runtime_error(sqlite3_errmsg(db));
    const auto course=sqlite3_column_int64(a.get(),0);
    a.reset();
    if(!lecturerAssigned(db,lecturer,course))
        throw std::invalid_argument("Lecturer is not assigned to this course");
    if(!studentEnrolled(db,student,course))
        throw std::invalid_argument("Student is not actively enrolled");
    const std::string current=effective(db,assignment,student);
    if(deadline<=std::string_view(current))
        throw std::invalid_argument("Extension must be later than current effective deadline");
    auto s=prepare(db,
        "INSERT INTO deadline_extensions(assignment_id,student_id,lecturer_id,new_deadline_utc) "
        "VALUES(?1,?2,?3,?4) "
        "ON CONFLICT(assignment_id,student_id) DO UPDATE SET "
        "lecturer_id=excluded.lecturer_id,new_deadline_utc=excluded.new_deadline_utc;");
    bindId(s.get(),1,assignment);
    bindId(s.get(),2,student);
    bindId(s.get(),3,lecturer);
    bindText(s.get(),4,deadline);
    if(sqlite3_step(s.get())!=SQLITE_DONE)throw std::runtime_error(sqlite3_errmsg(db));
    s.reset();
    tx.commit();
}

std::string SQLiteRecordRepository::findEffectiveDeadline(
    std::int64_t assignment,std::int64_t student) const
{
    return effective(database_.connection(),assignment,student);
}
} // namespace lms
