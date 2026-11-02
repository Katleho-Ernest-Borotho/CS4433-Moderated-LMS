# CR1 — v1.1 Software Design Delta (supplement to approved Week 3 v1.0 SDD)

**Change request:** CR1, whole-class assignment deadline extension  
**Baseline:** Approved v1.0 Week 3 design and twelve-table SQLite schema  
**Maintenance branch:** `feature/phase8-cr1-class-extensions`  
**Authoritative implementation:** `src/application/SubmissionManagementService.cpp`,
`src/persistence/SQLiteAssignmentsRepository.cpp`,
`src/persistence/SQLiteSubmissionRepository.cpp`,
`src/persistence/SQLiteModerationRepository.cpp`,
`src/persistence/SqliteDatabase.cpp`, and `src/ui/AssignmentPanel.cpp`.

This supplement documents **only CR1 changes**. Original Week 3 component,
class and ERD diagrams continue to represent the unchanged v1.0 baseline.
All diagrams below use native editable diagrams.net (`.drawio`) format,
with light backgrounds for readability.

## A. Updated editable drawings

- [CR1 v1.1 ERD delta](diagrams/CR1_ERD_v1_1.drawio)
- [CR1 v1.1 service/class delta](diagrams/CR1_SERVICE_CLASS_DELTA_v1_1.drawio)

To edit, open diagrams.net / draw.io and choose **File → Open from → Device**.
Both drawings retain editable nodes, text and connectors.

## B. ERD changes

The `class_deadline_extensions` table is added to the existing 12-table
database; no existing table is dropped or rebuilt.

| Field | SQLite type and constraint | Purpose |
|---|---|---|
| class_extension_id | INTEGER PRIMARY KEY | New class-extension identifier |
| assignment_id | INTEGER NOT NULL UNIQUE, FK to assignments | One current class extension per assignment |
| lecturer_id | INTEGER NOT NULL, FK to users | The lecturer who granted/updated it |
| new_deadline_utc | TEXT NOT NULL | New whole-class UTC deadline |
| created_at_utc | TEXT NOT NULL | UTC time of latest class-extension update |

**Relationships:** `assignments (1) → (0..1) class_deadline_extensions`;
`users [Lecturer] (1) → (0..many) class_deadline_extensions`.
The `deadline_extensions` table continues to store student-specific deadlines
unchanged.

The migration is executed transactionally by
`SqliteDatabase::initializeSchemaFromFile()` and is mirrored in
`schema/migrations/001_class_deadline_extensions.sql`. The final database
contains 13 application-defined tables.

## C. Changed classes and interfaces

| Component | Added/changed contract | Responsibility |
|---|---|---|
| AssignmentPanel (Qt Widgets) | `grantClassExtension()`; class deadline picker and separate Class due column | Lecturer action and Student-visible dates |
| ISubmissionService | `grantClassExtension(...)`, `classExtensionDeadline(...)` | Role-aware service contract |
| SubmissionManagementService | Implements new methods, validates UTC, Lecturer/course assignment | Authorised write and visibility |
| IAcademicRepository | `saveClassExtension(...)`, `findClassExtensionDeadline(...)` | Persistence abstraction |
| SQLiteRecordRepository | Implements class extension SQL transaction | Writes record with assignment scope and marking lock |
| SQLiteSubmissionRepository | Adjusts effective date lookup | Atomic submission/resubmission deadline |
| MarkingService | Uses existing `findEffectiveDeadline(...)` | Late penalty based on current effective date |
| SQLiteModerationRepository | Includes class deadline in HoD query | Prevents premature approval/release |

## D. Time and precedence rule

For assignment `A` and student `S`:

```text
effective_due(A,S) =
  max(A.original_due, A.current_class_extension, S.current_individual_extension)
```

Missing optional class/individual extensions default to the original deadline.
All stored dates use validated ISO-8601 UTC timestamps; Qt converts them to
local display format.

Examples:

| Original | Class extension | Individual extension | Effective deadline |
|---|---|---|---|
| 28 Oct | Not set | Not set | 28 Oct |
| 28 Oct | 30 Oct | Not set | 30 Oct |
| 28 Oct | 30 Oct | 3 Nov | 3 Nov |
| 28 Oct | 30 Oct | 29 Oct | 30 Oct |

## E. Behavioural sequence

1. Assigned Lecturer selects an assignment and later class date.
2. `AssignmentPanel` displays the confirmation dialogue.
3. `SubmissionManagementService` revalidates actor, role, UTC and course assignment.
4. `SQLiteRecordRepository` uses a transaction to verify the selected
   assignment, compare the new date, and check that no marks exist.
5. Database inserts/updates its `class_deadline_extensions` row.
6. Students refresh the assignment list. Each sees the same Class due date,
   plus their individual effective deadline in Your due.
7. Submission, marking and HoD moderation continue to apply the effective
   deadline consistently.

**Safety rule:** once an assessment result exists for any submission on
the assignment, the whole-class deadline cannot be extended further.
The source blocks a class extension that would silently alter already
recorded late penalties or released results.

## F. Quality assurance and acceptance

- Local v1.1 build and automated tests: **128/128 PASS**, 53.26 seconds.
- GitHub Windows UCRT64 CI for the implementation checkpoint: **successful**.
- Developer reported that the new v1.1 GUI works.
- New class-extension automated tests: **13**.
- v1.0 data migration retains existing users and records.
- **Still required:** timestamped GUI screenshots, independent v1.1 package
  launch on a second Windows laptop, and final supervisor approval.

**Version-control rule:** Do not replace the approved v1.0 Week 3 diagrams
with these diagrams. Submit the originals as the baseline and this document
as a traceable maintenance/change-control supplement.
