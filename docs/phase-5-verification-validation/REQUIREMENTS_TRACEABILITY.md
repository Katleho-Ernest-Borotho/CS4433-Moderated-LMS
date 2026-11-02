# CS4433 Requirements Traceability Matrix — LMS v1.0

**Baseline:** Week 2 SRS (FR-01..FR-18, NFR-01..NFR-06; all FR prioritised Must),
Week 3 Software Design Document. This table maps the *existing implementation*
to tests and verification. A mapped test is **not automatically a passing test**;
record its result using the Gate 3 evidence sheet.

## Functional requirements

| ID | User goal / SRS requirement | Code / design element | Representative automated test or planned manual check |
|---|---|---|---|
| FR-01 | Authenticate all four roles, show role dashboard | `AuthenticationService`, `LoginWindow`, `User` hierarchy, `users` | `AuthenticationTests.CanCreateAndLogInAsFirstAdministrator`, `UserManagementTests.AdministratorCanCreateAllFourRolesAndListAccounts`; ST-01 |
| FR-02 | Enforce role and course/department scope | `AuthenticationService::requireRole`, academic/marking/moderation services | `AuthenticationTests.ChecksRoleOnEveryProtectedServiceCall`, `ModerationTests.StudentsLecturersAndAdminCannotModerateOrRelease`; ST-02 |
| FR-03 | Create, list, update, deactivate user accounts | `AcademicManagementService`, `SQLiteRecordRepository`, `LoginWindow` | `AccountRecoveryTests.AdministratorCanRenameAccountWithoutLosingRecords`, `UserManagementTests.DeactivatedStudentIsListedInactiveAndCannotLogin`; ST-03 |
| FR-04 | Create/update/manage departments | `AcademicManagementService`, `AcademicPanel`, `departments` | `UserManagementTests.AdministratorCreatesUpdatesAndListsDepartmentsAndCourses`; ST-04 |
| FR-05 | Create/update/manage courses | `AcademicManagementService`, `AcademicPanel`, `courses` | `UserManagementTests.AdministratorCreatesUpdatesAndListsDepartmentsAndCourses`; ST-04 |
| FR-06 | Assign lecturers | `AcademicManagementService::assignLecturer`, `teaching_assignments` | `UserManagementTests.LecturerCanOnlySeeAssignedCourses`; ST-05 |
| FR-07 | Enrol students | `AcademicManagementService::enrolStudent`, `enrollments` | `UserManagementTests.StudentSeesOnlyOwnEnrolledCourses`; ST-05 |
| FR-08 | Lecturer creates assignment (title/deadline/max) | `AcademicManagementService::createAssignment`, `AssignmentPanel`, `assignments` | `AssignmentAndExtensionTests.AssignedLecturerCanCreateAssignment`; ST-06 |
| FR-09 | Lecturer grants individual extension | `SubmissionManagementService::grantExtension`, `deadline_extensions` | `AssignmentAndExtensionTests.LecturerGrantsIndividualExtensionOnlyToEnrolledStudent`; ST-06 |
| FR-10 | Student sees own courses/assignments | `AcademicManagementService::listVisibleCourses`, `AssignmentPanel` | `AssignmentAndExtensionTests.EnrolledStudentCanSeeAssignmentButOtherCannot`; ST-07 |
| FR-11 | Submit text/file/both with timestamp | `SubmissionManagementService`, `LocalSubmissionContentStore`, `submission_versions` | `SubmissionTests.StudentCanSubmitTextAndReviewFirstVersion`, `SubmissionTests.AttachmentIsCopiedAndCanBeRead`; ST-08 |
| FR-12 | Resubmit on/before effective deadline, retain history | `SubmissionManagementService`, `SQLiteSubmissionRepository` | `SubmissionTests.OnTimeResubmissionRetainsEarlierVersion`, `SubmissionTests.LateReplacementIsRejectedWithoutChangingHistory`; ST-08 |
| FR-13 | First late attempt, commenced-day penalty and 72h cutoff | `CommencedDayPenaltyPolicy`, `MarkingService` | `LatePenaltyPolicyTests.ExactBoundariesAndCommencedDays`, `MarkingTests.MoreThan72HoursLateForcesFinalMarkZero`; ST-09 |
| FR-14 | Lecturer reviews and marks assigned submissions with feedback | `MarkingService`, `MarkingPanel`, `assessment_results` | `MarkingTests.LecturerCanViewLatestSubmittedVersionAndSaveProvisionalMark`; ST-09 |
| FR-15 | HoD moderates only their department | `ModerationService`, `ResultsPanel`, `hod_departments` | `ModerationTests.AnotherDepartmentHoDCannotSeeApproveReleaseOrReadAttachments`; ST-10 |
| FR-16 | HoD releases approved marks only | `ModerationService::release`, `SQLiteModerationRepository` | `ModerationTests.ReleasingWithoutModerationIsRejected`, `ModerationTests.HoDApprovalThenSeparateReleaseMakesMarkVisibleToOnlyOwnStudent`; ST-10 |
| FR-17 | Student views own released marks and feedback only | `ModerationService::getReleasedResults`, Student results panel | `ModerationTests.ApprovalDoesNotReleasePrivateStudentMarks`, `ModerationTests.HoDApprovalThenSeparateReleaseMakesMarkVisibleToOnlyOwnStudent`; ST-11 |
| FR-18 | Audit every numerical mark change | `SQLiteAssessmentRepository`, `SQLiteModerationRepository`, `mark_change_audits` | `MarkingTests.ChangingEarnedAndFinalMarksCreatesAtomicAuditEntries`, `ModerationTests.FailedAuditInsertRollsBackHoDMarkAndApproval`; ST-12 |

Additional cross-role regression scenarios:
`ModerationTests.EndToEndStudentLecturerHoDAdministratorWorkflow` and
`ModerationTests.EndToEndDepartmentPrivacyAndAuditOnAdjustedRelease`.

## Non-functional requirements

| ID | Criterion from Week 2 SRS | Evidence / present status |
|---|---|---|
| NFR-01 | At least 19/20 valid logins complete in 2 seconds with 800 stored Student accounts | **Needs measurement** on target Windows device, 20 timings |
| NFR-02 | Core roles/workflows work offline | **Needs offline acceptance run** and recorded screenshots |
| NFR-03 | No plaintext passwords, all salted hashes | `PasswordTests.SaltedHashesVerifyWithoutStoringPlaintext` and repository inspection; verify actual database sample |
| NFR-04 | One tester per role completes task without help after <=15 min induction | **Needs four-tester usability record** |
| NFR-05 | All primary role operations keyboard accessible (Should) | **Needs keyboard-only inspection**, log defects |
| NFR-06 | All 20 test records of submissions, marks and audit survive restart unchanged | **Needs 20-record restart experiment** (partial database tests already exist) |

## Document consistency checks

- v1.0 runs offline on one Windows school PC, Qt6/C++17/SQLite, no online client.
- Use the *submitted* Week 2 SRS and Week 3 diagrams as the source of truth.
- Version 1.0 permits **individual** extensions only. Whole-class extension
  belongs to maintenance change **CR1 / version 1.1**, not the v1.0 baseline.
- The approved 12-table schema is retained.
- Role/permission rules must be enforced in the service/repository, not only by
  hiding GUI buttons.
- Generated counts and test names need to be rechecked against a specific
  commit immediately before final submission.
- No user-interface screenshots, measured coverage, full accessibility results,
  or Gate 3 supervisor approval are asserted in this matrix.

The final report should include: requirement → source/design → test name →
actual test result → evidence reference, not only a list of implemented features.
