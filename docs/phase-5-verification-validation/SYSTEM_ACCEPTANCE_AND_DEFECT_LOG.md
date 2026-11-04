# System and Acceptance Testing — Execution Record

**Project:** Moderated LMS v1.0 | **Phase:** 5 — V&V

Tester: __________________  Date: ______________  Git commit: ______________
Computer/OS: __________________  Offline test: Yes / No
Build type: Debug / Release  Test database: synthetic / demonstration data only

> Template only. Do not enter PASS until the operation is actually performed
> and its evidence saved. Screenshot names must correspond to real files.

## A. System and exception-flow scenarios

| ID | Role / setup | Test steps | Expected observable outcome | Actual result, PASS/FAIL, screenshot |
|---|---|---|---|---|
| ST-01 | All four roles | Attempt valid login and invalid password for each role | Correct dashboard; invalid password rejected without revealing account details | |
| ST-02 | Student/other Lecturer | Try actions of Administrator or HoD; attempt access to unassigned courses | Denied in GUI and application service | |
| ST-03 | Administrator | Create, rename, deactivate and reactivate test user; reset password | Changes persist; inactive/old credentials blocked | |
| ST-04 | Administrator | Create/update department and course; restart | IDs, links and names persist | |
| ST-05 | Administrator + Lecturer + Student | Assign lecturer to course; enrol student; sign in both | Only their assigned/enrolled courses visible | |
| ST-06 | Lecturer | Create assignment, grant later individual extension to enrolled Student; try unauthorized extension | Saved assignment; extended deadline only for named Student; unauthorized request rejected | |
| ST-07 | Student | View course and assignment list with effective deadline | Sees only enrolled course; local date/time displayed correctly | |
| ST-08 | Student | Submit text, file and both; replace before deadline; try after | Correct versions preserved; empty/oversized file rejected; late replacement rejected | |
| ST-09 | Lecturer | Mark latest version, add feedback, test late 24/48/72h boundaries | Correct deduction and provisional mark; wrong lecturer denied | |
| ST-10 | HoD | Review course; attempt premature release; approve; then release | Wrong department/future deadline denied; separate approval required | |
| ST-11 | Student | View result before marking, after marking, after HoD approval and after release | No unreleased marks; own released mark + feedback visible; other Student's hidden | |
| ST-12 | Lecturer/HoD | Change existing earned mark with reason; deliberately test audit write failure | Actor/time/old/new/reason logged; invalid audit transaction rolled back | |
| ST-13 | Administrator | Create and verify a full backup | Has 12-table database, referenced attachments, SHA-256 manifest | |
| ST-14 | Offline technician | Close LMS, restore synthetic backup, restart | Restored records/attachments match backup; safety backup retained | |
| ST-15 | Administrator | Refresh report and export CSV | Accurate totals; CSV readable; no plaintext password or submitted text | |
| ST-16 | All roles | Disconnect Internet and repeat one main task per role | All required core workflows still work without network | |

## B. Acceptance testing (from the three fully dressed SRS use cases)

| UC | User | Main path | Exception path to demonstrate | Evidence |
|---|---|---|---|---|
| UC-01 Submit assignment | Student | Enrolled course → create first submission → review version | Missing file/empty content, late first attempt, late replacement blocked | |
| UC-02 Mark Submission | Lecturer | Assigned course → review submission → mark + feedback → provisional result | Unassigned Lecturer, invalid mark, missing submission, audit failure | |
| UC-03 Moderate and Release Marks | HoD, Student | HoD checks/adjusts + approves → explicitly releases → Student sees result | Wrong department, release unapproved result, early release, audit failure | |

## C. Required measurable NFR record

**NFR-01: 20 valid login timings with 800 Student accounts**

Machine: __________________  SQLite Student account count: ________

| Attempt | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|---|---|---|---|---|---|---|---|---|---|---|
| Seconds | | | | | | | | | | |

| Attempt | 11 | 12 | 13 | 14 | 15 | 16 | 17 | 18 | 19 | 20 |
|---|---|---|---|---|---|---|---|---|---|---|
| Seconds | | | | | | | | | | |

Number <= 2 seconds: ______ / 20. PASS only if at least 19/20.

**NFR-02:** Internet disconnected? ____  Core offline tests passed? ____

**NFR-03:** Salted hashes inspected in database? ____  Password never stored as plaintext? ____

**NFR-04:** Four users, one per role. Intro time and assisted/unassisted completion:

| Role | Intro minutes (<=15) | Task completed? | Assistance needed? | Result |
|---|---|---|---|---|
| Administrator | | | | |
| Lecturer | | | | |
| HoD | | | | |
| Student | | | | |

**NFR-05:** Keyboard-only paths checked? ____  Blocked controls/defect IDs: __________

**NFR-06:** Twenty persisted test records compared after restart:
Matched ______ / 20. PASS only if 20/20 unchanged. Difference record IDs: ______

## D. Defect log and retests

Use one row per defect. Never mark FIXED before repeating the failed test on
a newer commit.

| ID | Discovered in test | Severity (Critical/High/Medium/Low) | Reproduction steps / observed vs expected | Root cause | Fix commit | Retest date/result |
|---|---|---|---|---|---|---|
| D-001 | | | | | | |
| D-002 | | | | | | |
| D-003 | | | | | | |

## E. Gate 3 evidence register

Commit SHA: __________________

CTest: ______ passed, ______ failed, ______ skipped.
JUnit report file: __________________

Actual domain/application **line coverage**: ______ %.
Coverage output file: __________________
CI workflow URL / status: __________________

Cppcheck executed? ____  Findings: Critical __ / High __ / Warning __
Reviewed false-positive findings (with evidence): __________________

Risk register updated? ____   All Must cases completed? ____
Any open Critical/High defects? ____   Gate 3 supervisor decision: __________

Tested by: __________________   Reviewed by: __________________

**This document is not a test result.** Fill it with actual dated observations.
