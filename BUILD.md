# Build and Test the Moderated LMS

Development platform: MSYS2 UCRT64, GCC C++17, Qt 6 Widgets, SQLite, libsodium, CMake, Ninja and GoogleTest.

## Update the source code

Close the running LMS application. Open **Git Bash**:

```bash
cd /c/Users/borot/OneDrive/Desktop/CS4433-Moderated-LMS
git switch feature/phase4-implementation
git pull --ff-only origin feature/phase4-implementation
```

## Build and run the tests

Open **MSYS2 UCRT64**:

```bash
cd /c/Users/borot/OneDrive/Desktop/CS4433-Moderated-LMS
cmake -S . -B /c/dev/cs4433-lms-build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build /c/dev/cs4433-lms-build
ctest --test-dir /c/dev/cs4433-lms-build --output-on-failure
/c/dev/cs4433-lms-build/ModeratedLMS.exe
```

The source suite now contains 55 GoogleTest cases. This is a target, not a claim that the current commit has passed on the user's laptop. Report the **first error** from the build output if it fails.

## Existing data

The SQLite database remains under the Windows local application-data directory for `CS4433/ModeratedLMS`. Its original **12-table approved Week 3 schema is unchanged**; do not delete the database to update the program. The initial Administrator, Student, Lecturer and existing course records should remain in place.

## Readable deadline display

The assignment grid presents deadlines in the computer's local timezone, for example `31 Oct 2026, 03:22`, rather than raw `2026-10-31T01:22:48Z` values. Stored deadline timestamps remain in UTC for calculations. Existing original timestamps keep their exact seconds, available in the grid's hover tooltip. When creating a new deadline with the minute-based editor, seconds are set to `00`.

## Paired workflow: first submission (FR-11) and versioned resubmission (FR-12)

1. As Administrator, ensure a Lecturer is assigned to CS4433 and a Student is enrolled.
2. As that Lecturer, create an assignment and choose a future deadline.
3. As the Student, open **Assignments and extensions**, select CS4433 and choose the assignment under **Submit assignment**.
4. Type an answer, choose an optional PDF, DOCX or TXT file, or provide only a file. Attachments must be between 1 byte and 10 MiB. Click **Submit / Resubmit**.
5. Confirm that **Version 1** appears in the Student's submission-history grid.
6. Before the effective deadline, change the response and submit again; confirm **Version 2** is listed while **Version 1** remains.
7. Close and reopen the app to verify both versions persist in SQLite. The Student's saved text can be reviewed in the history panel. Uploaded attachments are stored locally and listed as **Saved**; opening/downloading them from the GUI is not implemented yet.

**Deadline rules:** The first submission is accepted even after the effective deadline, and is classified as Late. A resubmission/replacement is permitted only on or before the individual effective deadline. A lecturer's extension may permit a later replacement; previous versions remain immutable. The late-penalty calculation and marking module will be implemented in subsequent checkpoints.

## Security and content storage

- Service methods authenticate the Student role and check that the Student is actively enrolled in the assignment's course.
- A Student can view only their own submission-version history.
- SQL statements are parameterised and the submission and version rows are committed in one SQLite transaction.
- Files are copied to a private local storage directory, with random opaque names rather than the originally supplied filename.
- The application checks the attachment extension (PDF, DOCX or TXT) and the size limit; file-content validation and virus scanning are not yet implemented.
- Attachment file operations and SQLite transactions cannot be made fully atomic as one filesystem/SQL transaction. The implementation attempts cleanup if the database write fails; crash-recovery and orphan-file reconciliation are still pending.
- Use **demonstration data only** until backup/recovery and final security checks are complete.

## Remaining core requirements

Late penalty/marking (FR-13/FR-14), moderation and release (FR-15/FR-17), and complete mark-change auditing (FR-18) are not yet implemented. Full file-viewing, user-account editing, and some additional administrative functions also remain.


## Paired implementation checkpoint: Late penalties + lecturer marking

This checkpoint adds the **CommencedDayPenaltyPolicy** and a **Marking** tab for
lecturers. It does not implement HoD approval or result release.

### Marking walkthrough

1. Sign in as the Lecturer assigned to the course and open **Marking**.
2. Choose the assigned course and an assignment. The table lists students who
   have submitted work and the newest version of each submission.
3. Select a student to inspect the newest typed text. If the version contains an
   attachment, use **Save latest attached file...** to save a review copy.
4. Enter the earned mark (from zero to the assignment maximum) and written feedback,
   then choose **Save provisional mark**.
5. The screen shows the earned mark, calculated late penalty and provisional final
   mark. These figures remain **hidden from students**.
6. To revise an existing mark or re-mark a more recent student version, enter a
   **revision reason**. Changed earned, penalty and final values are each recorded
   as separate audit events in the same SQLite transaction.
7. Log in as the Student. The Student still sees submissions and deadline details,
   but **not** the unreleased mark or feedback.

### Exact penalty boundaries

* On time, or exactly at the effective deadline: zero penalty.
* Late by (0, 24] hours: deduct 10% of the **maximum mark**.
* Late by (24, 48] hours: deduct 20% of the maximum mark.
* Late by (48, 72] hours: deduct 30% of the maximum mark.
* More than 72 hours late: final mark is zero.

For the first three late windows, the result is clamped to zero if the deduction
exceeds the earned mark. Marks are rounded to two decimal places. An individual
extension changes the effective deadline used in this computation.

### Re-marking after resubmission

If a Student submits a new version before their effective deadline, the earlier
mark is stale: a Lecturer must assess the **latest** version again before further
moderation. The service rejects marking a version that has been superseded.
Revisions to existing numerical marks and their audit events are written in one
database transaction. Existing version-history records are retained.

### Verification

Run the same Git Bash pull / MSYS2 UCRT64 CMake, build, CTest and launch commands
shown above. The suite now contains **70 test cases** including penalty boundaries,
valid and invalid marks, lecturer authorisation, file review, provisional status,
audit log revisions and resubmission re-marking.

This commit is committed to GitHub but has not yet been built on the development
machine. Do **not** treat the test target as a confirmed test result until CTest
reports success.

### Not implemented yet

HoD moderation, approval, mark adjustment by HoD, release of marks, and released
results in the Student view are still pending. Do not use real students' marks
until these and final backup/security checks are complete.
