# Phase 5 — Verification and Validation Test Plan

## 1. Purpose and scope

The test plan verifies **Moderated Learning Management System v1.0** against the submitted
Week 2 SRS (FR-01 to FR-18, NFR-01 to NFR-06 and BR-01 to BR-08) and the
approved Week 3 desktop design. This document is a *plan*. An automated test's
existence is not evidence of a pass; the test execution report and real screenshots
must be retained.

Platform: Windows desktop, MSYS2 UCRT64 GCC C++17, Qt6 Widgets, SQLite 3,
libsodium, GoogleTest, CMake/Ninja. A second Windows computer must be used
for independent deployment testing. No network connection is required by v1.0.

## 2. V-Model coverage

| Level | Verifies | Test method and evidence | Entry criteria | Exit criteria |
|---|---|---|---|---|
| Unit | Penalty policy, validation, account security, date boundaries, storage helpers | GoogleTest policy/validation tests, CTest XML, defect log | Classes compile and units have defined interfaces | Applicable unit assertions pass |
| Integration | Services, role checks, repository transactions, attachments, mark audit and release | GoogleTest cross-layer tests including EndToEndStudentLecturerHoDAdministratorWorkflow; SQLite data snapshots | Integrated service/database build succeeds | All integration assertions pass; no unintended data loss |
| System | Installed GUI and local database, all four role screens, backup/restore, offline operation | Manual system scripts ST-01–ST-12, screenshots, logs | Packaged application launches with test data | All mandatory system scenarios pass; major defects closed |
| Acceptance | User goals from three SRS use cases, usability and actual target hardware | Four role-based demonstrations, 20 timed logins with 800 accounts, 20-record persistence, acceptance sign-off | Gate 3 readiness; school-like data; independent test device | All Must acceptance criteria met and signed off |

## 3. Partitions and boundaries

| Category | Valid/invalid partitions | Boundary checks |
|---|---|---|
| Login | Valid credentials for four roles / unknown, wrong, deactivated account | Username 2/3/40/41 chars; password 11/12/1024/1025 bytes |
| Lateness | On time; first late attempt; permitted early replacement; refused late replacement | Exactly deadline, +1 sec, +24h, +24h 1sec, +48h, +48h 1sec, +72h, +72h 1sec |
| Earned mark | 0 through maximum; invalid negative, above maximum, nonfinite | 0, maximum, maximum+0.01; 2-decimal rounding |
| Individual extension | Assigned lecturer + enrolled student / unassigned lecturer or unenrolled student | Later versus equal/shortened deadline |
| Submission | Text; allowed attachment; both / empty, wrong extension, oversized | 1 byte, 10 MiB, 10 MiB+1 byte; valid/latest version |
| Moderation | Assigned HoD, marked latest version, past effective deadline / wrong department, future deadline, stale mark | Approved but not released; only release after approval |
| Persistence | Committed submissions, versions, marks and audits survive restart | 20 committed records; byte-for-byte values before/after |
| Backup | Complete database + attachments / tampered manifest or file | Missing referenced attachment; rejected restore while LMS open |

## 4. Measurable non-functional checks

**NFR-01 Performance.** Store 800 Student accounts in the target database.
Time 20 valid logins using a monotonic clock. At least 19/20 must be at or below
2.000 seconds. Record PC model, CPU, RAM, storage, OS, date, 20 timings
and the count satisfying the threshold. Do not infer a result from a few logins.

**NFR-02 Offline operation.** Disconnect Wi-Fi/Ethernet and repeat the core
Administrator, Lecturer, Student and HoD workflows, including local file
submission and released results. Note any network dependencies.

**NFR-03 Hash security.** Check all user rows for salted password hashes and
absence of plaintext storage. Verify libsodium Argon2id password hashing and
different hashes for identical passwords in tests.

**NFR-04 Usability.** Give four role testers no more than 15 minutes'
introduction, then ask each to complete one predefined normal task unassisted.
Record assistance required, completion, and time; do not invent observations.

**NFR-05 Keyboard accessibility (Should).** Perform primary actions with
Tab/Shift+Tab/Enter/Space and keyboard-only selection. Record blocked controls
and make a defect if an essential workflow is inaccessible.

**NFR-06 Persistence.** Write 20 known test records covering submission versions,
marks and mark audits. Close the LMS normally and restart it. Compare every
saved field and its record ID; all 20 must be unchanged.

## 5. Automation and line coverage

The GitHub Actions workflow `.github/workflows/ci.yml` builds the Windows
UCRT64 application, runs CTest and generates XML/HTML line-coverage and
cppcheck reports for the domain and application code. It must be inspected on
the GitHub Actions page for actual results; the workflow file alone does
not constitute evidence that CI passed.

On the developer computer, open **MSYS2 UCRT64** and run:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcovr
cd /c/Users/borot/OneDrive/Desktop/CS4433-Moderated-LMS
cmake -S . -B /c/dev/cs4433-lms-coverage -G Ninja -DCMAKE_BUILD_TYPE=Debug -DLMS_ENABLE_COVERAGE=ON
cmake --build /c/dev/cs4433-lms-coverage
ctest --test-dir /c/dev/cs4433-lms-coverage --output-on-failure
gcovr -r . /c/dev/cs4433-lms-coverage --filter 'src/domain/' --filter 'src/application/' --txt --html-details /c/dev/cs4433-lms-coverage/business-coverage.html
```

Record the **actual business-logic line coverage (%)** shown by gcovr.
The project brief aims for at least **70%**. If the result is below 70%,
add coverage for the uncovered business-logic paths and rerun tests.
Never report a hypothetical coverage percentage as a measured figure.
The 70% target must be judged against the domain/application scope,
not inflated by excluding hard-to-test business logic.

## 5a. Reproducible 800-Student login benchmark (NFR-01)

Build the separate benchmark on the target development/school computer in
**MSYS2 UCRT64** (normal build, **not** coverage-instrumented):

```bash
cd /c/Users/borot/OneDrive/Desktop/CS4433-Moderated-LMS
cmake -S . -B /c/dev/cs4433-lms-build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DLMS_ENABLE_NFR_BENCHMARK=ON -DLMS_ENABLE_COVERAGE=OFF
cmake --build /c/dev/cs4433-lms-build --target lms_login_benchmark
cd /c/dev/cs4433-lms-build
./lms_login_benchmark.exe lms_schema.sql
```

The benchmark creates a unique disposable SQLite database in the temporary
directory, inserts **800 Student records**, and reports each of **20 actual
timed valid logins** and how many took <=2,000 ms. It uses the production
authentication/password-verification code path and does not touch the
real LMS accounts. The disposable fixture copies one salted synthetic
password hash to avoid expensive setup for 800 separate accounts; therefore
this is a timing benchmark, **not** an NFR-03 audit of unique password salts.
Use the real password-security tests and database inspection for NFR-03.

Keep the console output as evidence alongside the computer specifications,
timestamp and Git commit. A pass requires at least **19 out of 20** valid
logins at or below 2 seconds on the target machine.

## 6. Execution evidence and Gate 3 decision

Keep a dated test record with the Git commit SHA, test command, number of
passing/failing/skipped tests, actual coverage percentage, hardware used,
screenshots, defect log and re-test result.

**Entry:** Gate 2 approved; source builds; schema/fixtures installed; role
accounts prepared; risk of data loss controlled with a test database and backup.

**Exit for Gate 3:** All FR-01..FR-18 Must scenarios pass; tested
NFR-01/02/03/04/06 criteria pass; at least 70% measured business-logic
line coverage; no open Critical/High defects; supervisor review documented.

No numerical results, screenshots, usability observations, dates of execution
or supervisor sign-off are claimed by this test plan.
