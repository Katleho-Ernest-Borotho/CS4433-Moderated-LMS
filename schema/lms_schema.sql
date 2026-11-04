-- Moderated LMS v1.0 SQLite schema (approved Week 3 design).
PRAGMA foreign_keys = ON;

CREATE TABLE users (
 user_id INTEGER PRIMARY KEY,
 username TEXT NOT NULL COLLATE NOCASE UNIQUE CHECK(length(trim(username)) > 0),
 password_hash TEXT NOT NULL CHECK(length(trim(password_hash)) > 0),
 role TEXT NOT NULL CHECK(role IN ('Student','Lecturer','HeadOfDepartment','Administrator')),
 active INTEGER NOT NULL DEFAULT 1 CHECK(active IN (0,1))
);
CREATE TABLE departments (
 department_id INTEGER PRIMARY KEY,
 name TEXT NOT NULL COLLATE NOCASE UNIQUE CHECK(length(trim(name)) > 0)
);
CREATE TABLE hod_departments (
 hod_id INTEGER PRIMARY KEY REFERENCES users(user_id) ON DELETE RESTRICT,
 department_id INTEGER NOT NULL REFERENCES departments(department_id) ON DELETE RESTRICT
);
CREATE TABLE courses (
 course_id INTEGER PRIMARY KEY,
 department_id INTEGER NOT NULL REFERENCES departments(department_id) ON DELETE RESTRICT,
 code TEXT NOT NULL COLLATE NOCASE UNIQUE CHECK(length(trim(code)) > 0),
 title TEXT NOT NULL CHECK(length(trim(title)) > 0)
);
CREATE TABLE enrollments (
 student_id INTEGER NOT NULL REFERENCES users(user_id) ON DELETE RESTRICT,
 course_id INTEGER NOT NULL REFERENCES courses(course_id) ON DELETE RESTRICT,
 enrolled_at_utc TEXT NOT NULL,
 PRIMARY KEY(student_id,course_id)
);
CREATE TABLE teaching_assignments (
 lecturer_id INTEGER NOT NULL REFERENCES users(user_id) ON DELETE RESTRICT,
 course_id INTEGER NOT NULL REFERENCES courses(course_id) ON DELETE RESTRICT,
 PRIMARY KEY(lecturer_id,course_id)
);
CREATE TABLE assignments (
 assignment_id INTEGER PRIMARY KEY,
 course_id INTEGER NOT NULL REFERENCES courses(course_id) ON DELETE RESTRICT,
 title TEXT NOT NULL CHECK(length(trim(title)) > 0),
 deadline_utc TEXT NOT NULL,
 maximum_mark REAL NOT NULL CHECK(maximum_mark > 0)
);
CREATE TABLE deadline_extensions (
 extension_id INTEGER PRIMARY KEY,
 assignment_id INTEGER NOT NULL REFERENCES assignments(assignment_id) ON DELETE RESTRICT,
 student_id INTEGER NOT NULL REFERENCES users(user_id) ON DELETE RESTRICT,
 lecturer_id INTEGER NOT NULL REFERENCES users(user_id) ON DELETE RESTRICT,
 new_deadline_utc TEXT NOT NULL,
 UNIQUE(assignment_id, student_id)
);
CREATE TABLE submissions (
 submission_id INTEGER PRIMARY KEY,
 student_id INTEGER NOT NULL REFERENCES users(user_id) ON DELETE RESTRICT,
 assignment_id INTEGER NOT NULL REFERENCES assignments(assignment_id) ON DELETE RESTRICT,
 state TEXT NOT NULL CHECK(state IN ('Submitted','Late','Marked','Moderated','Released')),
 UNIQUE(student_id, assignment_id)
);
CREATE TABLE submission_versions (
 version_id INTEGER PRIMARY KEY,
 submission_id INTEGER NOT NULL REFERENCES submissions(submission_id) ON DELETE RESTRICT,
 version_number INTEGER NOT NULL CHECK(version_number >= 1),
 submitted_at_utc TEXT NOT NULL,
 text_content TEXT,
 attachment_ref TEXT,
 is_late INTEGER NOT NULL CHECK(is_late IN (0,1)),
 UNIQUE(submission_id, version_number),
 UNIQUE(submission_id, version_id),
 CHECK(length(trim(COALESCE(text_content,''))) > 0 OR length(trim(COALESCE(attachment_ref,''))) > 0)
);
CREATE TABLE assessment_results (
 result_id INTEGER PRIMARY KEY,
 submission_id INTEGER NOT NULL UNIQUE REFERENCES submissions(submission_id) ON DELETE RESTRICT,
 marked_version_id INTEGER NOT NULL,
 marked_by_lecturer_id INTEGER NOT NULL REFERENCES users(user_id) ON DELETE RESTRICT,
 earned_mark REAL NOT NULL CHECK(earned_mark >= 0),
 penalty_mark REAL NOT NULL CHECK(penalty_mark >= 0),
 final_mark REAL NOT NULL CHECK(final_mark >= 0),
 feedback TEXT NOT NULL CHECK(length(trim(feedback)) > 0),
 approved INTEGER NOT NULL DEFAULT 0 CHECK(approved IN (0,1)),
 moderated_by_hod_id INTEGER REFERENCES users(user_id) ON DELETE RESTRICT,
 moderated_at_utc TEXT,
 released_at_utc TEXT,
 FOREIGN KEY(submission_id,marked_version_id) REFERENCES submission_versions(submission_id,version_id) ON DELETE RESTRICT,
 CHECK(approved = 0 OR (moderated_by_hod_id IS NOT NULL AND moderated_at_utc IS NOT NULL)),
 CHECK(released_at_utc IS NULL OR approved = 1)
);
CREATE TABLE mark_change_audits (
 audit_id INTEGER PRIMARY KEY,
 result_id INTEGER NOT NULL REFERENCES assessment_results(result_id) ON DELETE RESTRICT,
 changed_by_user_id INTEGER NOT NULL REFERENCES users(user_id) ON DELETE RESTRICT,
 changed_at_utc TEXT NOT NULL,
 changed_field TEXT NOT NULL CHECK(changed_field IN ('earned_mark','penalty_mark','final_mark')),
 old_mark REAL NOT NULL CHECK(old_mark >= 0),
 new_mark REAL NOT NULL CHECK(new_mark >= 0),
 reason TEXT NOT NULL CHECK(length(trim(reason)) > 0)
);
CREATE INDEX idx_courses_department ON courses(department_id);
CREATE INDEX idx_enrollments_course ON enrollments(course_id);
CREATE INDEX idx_teaching_course ON teaching_assignments(course_id);
CREATE INDEX idx_assignments_course ON assignments(course_id);
CREATE INDEX idx_extensions_student ON deadline_extensions(student_id);
CREATE INDEX idx_submissions_assignment ON submissions(assignment_id);
CREATE INDEX idx_versions_submitted ON submission_versions(submission_id,submitted_at_utc);
CREATE INDEX idx_results_moderator ON assessment_results(moderated_by_hod_id);
CREATE INDEX idx_audits_result ON mark_change_audits(result_id,changed_at_utc);
