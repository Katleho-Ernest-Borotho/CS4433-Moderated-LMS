# Phase 8 — CR1: Whole-Class Deadline Extensions (v1.1)

## 1. Change request and baseline

**Request ID:** CR1

**Baseline:** The working, tested offline Windows Moderated LMS v1.0 with
individual Student extensions, versioned submissions, automatic commenced-day
late penalties, Lecturer marking, HoD moderation and controlled result release.

**Requested change:** A Lecturer assigned to a course can extend an assignment
deadline **for the whole class**, without manually extending every enrolled
Student. Existing individual deadline extensions remain valid.

**Change status:** v1.1 development implementation is in branch
`feature/phase8-cr1-class-extensions`. Requires compilation, 128-test
regression run and manual acceptance before merging/tagging.

## 2. Updated functional requirements

**FR-09.1 (CR1).** An assigned, active Lecturer can grant a later
whole-class deadline for an assignment in one action.

**FR-09.2 (CR1).** Each Student's effective deadline is the **latest of**:
the original assignment deadline, the class extension deadline and their
individual extension deadline. This retains longer personal accommodations.

**FR-09.3 (CR1).** The Student assignment list shows both the current
**Class due** and individual **Your due** dates, in local time. Lecturer
also sees class due and has a confirmed class-extension action.

**FR-09.4 (CR1).** Changing the class deadline must not retroactively
invalidate a stored mark or released result. Class extension creation
and further updates are rejected once marking starts for any Student on
the assignment.

**FR-09.5 (CR1).** First submissions, versioned replacements, lateness
calculation, marking and HoD release must all resolve the same effective
deadline, with the class extension considered.

## 3. Impact analysis

| Layer | Change | Regression risk |
|---|---|---|
| SQLite schema | Add `class_deadline_extensions` with assignment unique key, Lecturer ID, new deadline and audit timestamp | Existing on-disk v1.0 database must be preserved; migration repeated safely |
| Persistence | `SQLiteAssignmentsRepository` adds class extension write/read; effective deadline uses scalar `MAX` | Wrong student association or an individual extension shortened |
| Submission | `SQLiteSubmissionRepository` recalculates effective deadline inside transaction | Late/early replacement misclassification |
| Application | `SubmissionManagementService` verifies role, assignment and assigned Lecturer | Unauthorised changes or malformed timestamp |
| Marking | Existing `MarkingService` reads updated effective deadline through repository | Wrong late penalty |
| Moderation | HoD query includes class deadline in its projected effective deadline | Early release after new extension |
| GUI | Lecturer confirmation/date picker; Student class and personal due columns | Wrong timezone or unavailable operation |
| Backup/recovery | The database now has 13 user-defined tables; backup retains entire SQLite file | Backup/restore and v1.0 interoperability |
| Documentation/tests | New regression cases and acceptance evidence | Unverified release claims |

### Design and data decision

Rather than changing the original `assignments.deadline_utc` or
`deadline_extensions` semantics, the new table stores an independently
editable, course-wide deadline:

```sql
CREATE TABLE IF NOT EXISTS class_deadline_extensions (
    class_extension_id INTEGER PRIMARY KEY,
    assignment_id INTEGER NOT NULL UNIQUE
        REFERENCES assignments(assignment_id) ON DELETE RESTRICT,
    lecturer_id INTEGER NOT NULL
        REFERENCES users(user_id) ON DELETE RESTRICT,
    new_deadline_utc TEXT NOT NULL,
    created_at_utc TEXT NOT NULL
);
```

For a Student `S` and assignment `A`:

```text
effective_due(A,S) =
    latest(original_due(A), class_due(A), individual_due(A,S))
```

All deadlines are compared as validated ISO-8601 UTC timestamps. Missing
class/individual values do not override the original date.

This model changes the baseline from **12 to 13 application tables** on
v1.1. The original twelve tables are unchanged. The checked-in migration
SQL is in `schema/migrations/001_class_deadline_extensions.sql`; the
application executes the same DDL transactionally on first startup.

### Business rules and risk controls

- One current class extension per assignment; further extension moves
  it later, never earlier or to the same time.
- Only the **assigned Lecturer** can grant the change.
- Enrolled Students see the class-wide date, but outsiders cannot view it.
- Individual extensions that are later than the class deadline are retained.
- The Lecturer must confirm a whole-class change.
- **Marking lock:** If `assessment_results` already contains a mark for
  this assignment, any new class extension is blocked. This conservative
  safeguard prevents silent recalculation of stored penalties.
- Existing submission versions, audit history and released results are not
  rewritten by the migration.
- The system does not automatically send notifications (offline v1.0
  architecture). Students see the updated date on their next refresh.

## 4. Upgrade and rollback

**Mandatory precaution:** Close the LMS and create a **verified full backup**
on the working v1.0 build before first launching the v1.1 build on an
existing Windows user profile. Retain it outside the application-data folder.
Make sure the backup includes attachments.

The v1.1 startup executes `CREATE TABLE IF NOT EXISTS` inside a single
SQLite transaction, so the migration can be repeated without deleting data.
The v1.0 `lms_schema.sql` baseline remains unchanged; the v1.1 migration
is applied programmatically.

**Do not roll back by launching an older v1.0 binary against an already
migrated database** as an undocumented downgrade. For reliable rollback:
close v1.1, use the offline Restore utility to restore the verified v1.0
backup (both database and attachments), then run the v1.0 executable.

The separate branch must remain unmerged until tests and manual regression
complete. No actual tests are claimed in this change report.

## 5. Planned regression checklist

| Test | Expected |
|---|---|
| Class extension for two enrolled Students | Both gain later class deadline |
| Longer existing individual deadline | Personal due date remains later |
| Shorter individual deadline | Class due date becomes effective |
| Repeated later class extension | Updates successfully |
| Equal or earlier date | Rejected; stored dates unchanged |
| Unassigned Lecturer / Student tries class update | Access denied |
| Outside Student reads class deadline | Access denied |
| Class change affects other assignment | No |
| Class extension after Student first late submission | Resubmission allowed if new date is future |
| Penalty for a submission now within class deadline | No late deduction |
| Class deadline after any mark exists | Rejected |
| Upgrade v1.0 database twice | No duplicate table, existing accounts preserved |
| Backups and offline Restore after upgrade | Database and attachments preserved |
| Normal UI on second Windows laptop | Correct dates and no Qt runtime error |

## 6. Acceptance gate

1. Build the new branch with MSYS2 UCRT64.
2. Execute **128** GoogleTest cases and confirm **0 failed**.
3. Capture full Lecturer and Student GUI screenshots of CR1 operation.
4. Verify the individual extension and penalty tests remain passing.
5. Verify the migrated real test database retains existing data and
   the backup/restore scenario still works.
6. Update the class diagram, ERD and change log for the added table and
   related class/service interfaces.
7. Only after results are recorded, consider v1.1 merge/release.

**Status:** Code changes committed to maintenance branch. No claim of
passing v1.1 build or acceptance tests until executed.
