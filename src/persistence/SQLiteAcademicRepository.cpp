#include "lms/persistence/SQLiteRecordRepository.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace lms {
namespace {
using Statement = std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;

Statement statement(sqlite3* db, const char* sql)
{
    sqlite3_stmt* raw = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &raw, nullptr) != SQLITE_OK)
        throw std::runtime_error(sqlite3_errmsg(db));
    return Statement(raw, &sqlite3_finalize);
}

void bindText(sqlite3_stmt* statement, int index, std::string_view text)
{
    if (sqlite3_bind_text(statement, index, text.data(), static_cast<int>(text.size()),
                          SQLITE_TRANSIENT) != SQLITE_OK)
        throw std::runtime_error("Unable to bind SQL parameter");
}

void bindId(sqlite3_stmt* statement, int index, std::int64_t id)
{
    if (sqlite3_bind_int64(statement, index, id) != SQLITE_OK)
        throw std::runtime_error("Unable to bind SQL identifier");
}

std::string columnText(sqlite3_stmt* statement, int index)
{
    auto* value = sqlite3_column_text(statement, index);
    return value ? reinterpret_cast<const char*>(value) : "";
}

void exec(sqlite3* db, const char* sql)
{
    char* message = nullptr;
    const int result = sqlite3_exec(db, sql, nullptr, nullptr, &message);
    if (result != SQLITE_OK) {
        std::string reason = message ? message : sqlite3_errmsg(db);
        sqlite3_free(message);
        throw std::runtime_error(reason);
    }
}

class WriteTransaction {
public:
    explicit WriteTransaction(sqlite3* db) : db_(db) { exec(db_, "BEGIN IMMEDIATE;"); }
    void commit() { exec(db_, "COMMIT;"); committed_ = true; }
    ~WriteTransaction() {
        if (!committed_) sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    }
    WriteTransaction(const WriteTransaction&) = delete;
    WriteTransaction& operator=(const WriteTransaction&) = delete;
private:
    sqlite3* db_;
    bool committed_ = false;
};

void runWrite(sqlite3* db, sqlite3_stmt* prepared)
{
    if (sqlite3_step(prepared) != SQLITE_DONE)
        throw std::runtime_error(sqlite3_errmsg(db));
}

void ensureExisting(sqlite3* db, const char* query, std::int64_t id, const char* reason)
{
    auto prepared = statement(db, query);
    bindId(prepared.get(), 1, id);
    if (sqlite3_step(prepared.get()) != SQLITE_ROW ||
        sqlite3_column_int(prepared.get(), 0) != 1)
        throw std::invalid_argument(reason);
}

void ensureRole(sqlite3* db, std::int64_t id, const char* role)
{
    auto prepared = statement(db,
        "SELECT EXISTS(SELECT 1 FROM users WHERE user_id=?1 AND role=?2 AND active=1);");
    bindId(prepared.get(), 1, id);
    bindText(prepared.get(), 2, role);
    if (sqlite3_step(prepared.get()) != SQLITE_ROW ||
        sqlite3_column_int(prepared.get(), 0) != 1)
        throw std::invalid_argument("Select an active user with the required role");
}

void ensureDepartment(sqlite3* db, std::int64_t id)
{
    ensureExisting(db,
        "SELECT EXISTS(SELECT 1 FROM departments WHERE department_id=?1);",
        id, "Department does not exist");
}

void ensureCourse(sqlite3* db, std::int64_t id)
{
    ensureExisting(db,
        "SELECT EXISTS(SELECT 1 FROM courses WHERE course_id=?1);",
        id, "Course does not exist");
}

std::vector<Course> listCourses(sqlite3* db, const char* sql, std::int64_t id, bool filter)
{
    auto prepared = statement(db, sql);
    if (filter) bindId(prepared.get(), 1, id);
    std::vector<Course> courses;
    for (;;) {
        const int result = sqlite3_step(prepared.get());
        if (result == SQLITE_DONE) return courses;
        if (result != SQLITE_ROW) throw std::runtime_error(sqlite3_errmsg(db));
        courses.push_back(Course{sqlite3_column_int64(prepared.get(),0),
                                 sqlite3_column_int64(prepared.get(),1),
                                 columnText(prepared.get(),2), columnText(prepared.get(),3)});
    }
}
} // namespace

Department SQLiteRecordRepository::saveDepartment(std::string_view name)
{
    sqlite3* db = database_.connection();
    WriteTransaction write(db);
    auto prepared = statement(db, "INSERT INTO departments(name) VALUES(?1);");
    bindText(prepared.get(), 1, name);
    runWrite(db, prepared.get());
    const auto id = sqlite3_last_insert_rowid(db);
    prepared.reset();
    write.commit();
    return Department{id, std::string(name)};
}

void SQLiteRecordRepository::updateDepartment(const Department& department)
{
    sqlite3* db = database_.connection();
    WriteTransaction write(db);
    auto prepared = statement(db, "UPDATE departments SET name=?1 WHERE department_id=?2;");
    bindText(prepared.get(), 1, department.name);
    bindId(prepared.get(), 2, department.id);
    runWrite(db, prepared.get());
    if (sqlite3_changes(db) != 1) throw std::invalid_argument("Department does not exist");
    prepared.reset();
    write.commit();
}

std::vector<Department> SQLiteRecordRepository::listDepartments() const
{
    sqlite3* db = database_.connection();
    auto prepared = statement(db,
        "SELECT department_id,name FROM departments ORDER BY name COLLATE NOCASE;");
    std::vector<Department> results;
    for (;;) {
        const int result = sqlite3_step(prepared.get());
        if (result == SQLITE_DONE) return results;
        if (result != SQLITE_ROW) throw std::runtime_error(sqlite3_errmsg(db));
        results.push_back(Department{sqlite3_column_int64(prepared.get(),0),
                                     columnText(prepared.get(),1)});
    }
}

Course SQLiteRecordRepository::saveCourse(std::int64_t departmentId, std::string_view code,
                                         std::string_view title)
{
    sqlite3* db = database_.connection();
    WriteTransaction write(db);
    ensureDepartment(db, departmentId);
    auto prepared = statement(db,
        "INSERT INTO courses(department_id,code,title) VALUES(?1,?2,?3);");
    bindId(prepared.get(), 1, departmentId);
    bindText(prepared.get(), 2, code);
    bindText(prepared.get(), 3, title);
    runWrite(db, prepared.get());
    const auto id = sqlite3_last_insert_rowid(db);
    prepared.reset();
    write.commit();
    return Course{id,departmentId,std::string(code),std::string(title)};
}

void SQLiteRecordRepository::updateCourse(const Course& course)
{
    sqlite3* db = database_.connection();
    WriteTransaction write(db);
    ensureDepartment(db,course.departmentId);
    auto prepared = statement(db,
        "UPDATE courses SET department_id=?1,code=?2,title=?3 WHERE course_id=?4;");
    bindId(prepared.get(),1,course.departmentId);
    bindText(prepared.get(),2,course.code);
    bindText(prepared.get(),3,course.title);
    bindId(prepared.get(),4,course.id);
    runWrite(db,prepared.get());
    if (sqlite3_changes(db) != 1) throw std::invalid_argument("Course does not exist");
    prepared.reset();
    write.commit();
}

std::vector<Course> SQLiteRecordRepository::listAllCourses() const
{
    return listCourses(database_.connection(),
        "SELECT course_id,department_id,code,title FROM courses ORDER BY code COLLATE NOCASE;",
        0, false);
}

void SQLiteRecordRepository::assignHoD(std::int64_t hodId, std::int64_t departmentId)
{
    sqlite3* db = database_.connection();
    WriteTransaction write(db);
    ensureRole(db,hodId,"HeadOfDepartment");
    ensureDepartment(db,departmentId);
    auto prepared = statement(db,
        "INSERT INTO hod_departments(hod_id,department_id) VALUES(?1,?2);");
    bindId(prepared.get(),1,hodId);
    bindId(prepared.get(),2,departmentId);
    runWrite(db,prepared.get());
    prepared.reset();
    write.commit();
}

void SQLiteRecordRepository::assignLecturer(std::int64_t lecturerId, std::int64_t courseId)
{
    sqlite3* db = database_.connection();
    WriteTransaction write(db);
    ensureRole(db,lecturerId,"Lecturer");
    ensureCourse(db,courseId);
    auto prepared = statement(db,
        "INSERT INTO teaching_assignments(lecturer_id,course_id) VALUES(?1,?2);");
    bindId(prepared.get(),1,lecturerId);
    bindId(prepared.get(),2,courseId);
    runWrite(db,prepared.get());
    prepared.reset();
    write.commit();
}

void SQLiteRecordRepository::enrolStudent(std::int64_t studentId, std::int64_t courseId,
                                          std::string_view enrolledAtUtc)
{
    sqlite3* db = database_.connection();
    WriteTransaction write(db);
    ensureRole(db,studentId,"Student");
    ensureCourse(db,courseId);
    auto prepared = statement(db,
        "INSERT INTO enrollments(student_id,course_id,enrolled_at_utc) VALUES(?1,?2,?3);");
    bindId(prepared.get(),1,studentId);
    bindId(prepared.get(),2,courseId);
    bindText(prepared.get(),3,enrolledAtUtc);
    runWrite(db,prepared.get());
    prepared.reset();
    write.commit();
}

std::vector<Course> SQLiteRecordRepository::listStudentCourses(std::int64_t studentId) const
{
    return listCourses(database_.connection(),
        "SELECT c.course_id,c.department_id,c.code,c.title FROM courses AS c "
        "JOIN enrollments AS e ON e.course_id=c.course_id "
        "WHERE e.student_id=?1 ORDER BY c.code COLLATE NOCASE;",studentId,true);
}

std::vector<Course> SQLiteRecordRepository::listLecturerCourses(std::int64_t lecturerId) const
{
    return listCourses(database_.connection(),
        "SELECT c.course_id,c.department_id,c.code,c.title FROM courses AS c "
        "JOIN teaching_assignments AS t ON t.course_id=c.course_id "
        "WHERE t.lecturer_id=?1 ORDER BY c.code COLLATE NOCASE;",lecturerId,true);
}

std::vector<Course> SQLiteRecordRepository::listHoDCourses(std::int64_t hodId) const
{
    return listCourses(database_.connection(),
        "SELECT c.course_id,c.department_id,c.code,c.title FROM courses AS c "
        "JOIN hod_departments AS h ON h.department_id=c.department_id "
        "WHERE h.hod_id=?1 ORDER BY c.code COLLATE NOCASE;",hodId,true);
}
} // namespace lms
