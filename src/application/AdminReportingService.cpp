#include "lms/application/AdminReportingService.h"
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sstream>

namespace lms {
namespace {
using Statement=std::unique_ptr<sqlite3_stmt,decltype(&sqlite3_finalize)>;
Statement prepare(sqlite3* db,const char* sql) {
    sqlite3_stmt* raw=nullptr;
    if(sqlite3_prepare_v2(db,sql,-1,&raw,nullptr)!=SQLITE_OK)
        throw std::runtime_error(sqlite3_errmsg(db));
    return Statement(raw,&sqlite3_finalize);
}
std::int64_t count(sqlite3* db,const char* query) {
    auto s=prepare(db,query);
    if(sqlite3_step(s.get())!=SQLITE_ROW)throw std::runtime_error(sqlite3_errmsg(db));
    return sqlite3_column_int64(s.get(),0);
}
std::string value(sqlite3_stmt* s,int col) {
    const auto* data=sqlite3_column_text(s,col);
    return data?reinterpret_cast<const char*>(data):"";
}
std::string csvCell(std::string_view raw) {
    // Guard spreadsheet formula injection in database-controlled names and titles.
    std::string s(raw);
    const auto first=s.find_first_not_of(" \t\r\n");
    if(first!=std::string::npos) {
        const char c=s[first];
        if(c=='='||c=='+'||c=='-'||c=='@')
            s.insert(s.begin(),(char)39);
    }
    std::string escaped="\"";
    for(char c:s) {
        if(c=='\"')escaped+="\"\"";
        else escaped+=c;
    }
    return escaped+"\"";
}
} // namespace

AdminReportingService::AdminReportingService(
    const IAuthenticationService& authentication,sqlite3* database)
    : authentication_(authentication),db_(database) {
    if(!db_)throw std::invalid_argument("Reporting database is missing");
}
AdminReport AdminReportingService::generate(const Session& actor) const {
    authentication_.requireRole(actor,Role::Administrator);
    // Explicitly only aggregate administrative data; no password hashes or
    // submission content is returned through this reporting feature.
    AdminReport r;
    r.activeUsers=count(db_,"SELECT COUNT(*) FROM users WHERE active=1;");
    r.inactiveUsers=count(db_,"SELECT COUNT(*) FROM users WHERE active=0;");
    r.departments=count(db_,"SELECT COUNT(*) FROM departments;");
    r.courses=count(db_,"SELECT COUNT(*) FROM courses;");
    r.assignments=count(db_,"SELECT COUNT(*) FROM assignments;");
    r.submissions=count(db_,"SELECT COUNT(*) FROM submissions;");
    r.versions=count(db_,"SELECT COUNT(*) FROM submission_versions;");
    r.provisional=count(db_,
        "SELECT COUNT(*) FROM assessment_results "
        "WHERE approved=0 AND released_at_utc IS NULL;");
    r.approvedUnreleased=count(db_,
        "SELECT COUNT(*) FROM assessment_results "
        "WHERE approved=1 AND released_at_utc IS NULL;");
    r.released=count(db_,
        "SELECT COUNT(*) FROM assessment_results "
        "WHERE approved=1 AND released_at_utc IS NOT NULL;");
    auto s=prepare(db_,
        "SELECT c.code,c.title,d.name,"
        "(SELECT COUNT(*) FROM enrollments e WHERE e.course_id=c.course_id),"
        "(SELECT COUNT(*) FROM assignments a WHERE a.course_id=c.course_id),"
        "(SELECT COUNT(*) FROM submissions sub JOIN assignments a "
        "ON a.assignment_id=sub.assignment_id WHERE a.course_id=c.course_id),"
        "(SELECT COUNT(*) FROM assessment_results ar "
        "JOIN submissions sub ON sub.submission_id=ar.submission_id "
        "JOIN assignments a ON a.assignment_id=sub.assignment_id "
        "WHERE a.course_id=c.course_id AND ar.approved=1 "
        "AND ar.released_at_utc IS NOT NULL) "
        "FROM courses c JOIN departments d ON d.department_id=c.department_id "
        "ORDER BY c.code COLLATE NOCASE;");
    while(true) {
        const int rc=sqlite3_step(s.get());
        if(rc==SQLITE_DONE)break;
        if(rc!=SQLITE_ROW)throw std::runtime_error(sqlite3_errmsg(db_));
        r.courseRows.push_back({value(s.get(),0),value(s.get(),1),value(s.get(),2),
            sqlite3_column_int64(s.get(),3),sqlite3_column_int64(s.get(),4),
            sqlite3_column_int64(s.get(),5),sqlite3_column_int64(s.get(),6)});
    }
    return r;
}
std::string AdminReportingService::toCsv(const AdminReport& r) {
    std::ostringstream out;
    out<<"Record,Name,Value,Department,Enrolled,Assignments,Submissions,Released results\r\n";
    const auto metric=[&out](const char* name,std::int64_t number) {
        out<<"Metric,"<<csvCell(name)<<","<<number<<",,,,\r\n";
    };
    metric("Active accounts",r.activeUsers);
    metric("Inactive accounts",r.inactiveUsers);
    metric("Departments",r.departments);
    metric("Courses",r.courses);
    metric("Assignments",r.assignments);
    metric("Submissions",r.submissions);
    metric("Submission versions",r.versions);
    metric("Provisional results",r.provisional);
    metric("Approved, unreleased results",r.approvedUnreleased);
    metric("Released results",r.released);
    for(const auto& course:r.courseRows) {
        out<<"Course,"<<csvCell(course.code)<<","<<csvCell(course.title)<<","
           <<csvCell(course.department)<<","<<course.enrolled<<","
           <<course.assignments<<","<<course.submissions<<","
           <<course.releasedResults<<"\r\n";
    }
    return out.str();
}
} // namespace lms
