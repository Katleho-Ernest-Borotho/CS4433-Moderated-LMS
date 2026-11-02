# CR1 v1.1 — Developer Build, Migration and GUI Verification

**Branch:** `feature/phase8-cr1-class-extensions`

The v1.1 implementation is still under verification. It contains **128
GoogleTest cases**, including **13 CR1 regression cases**, in the current
source. Do not describe them as passed until CTest/CI reports success.

## A. Protect your v1.0 data first

**Required:** Using the *existing tested v1.0 program*, sign in as
Administrator → **Maintenance and reports** → **Create full backup**.
Choose a protected location outside the LMS app-data folder. Then click
**Verify backup folder** and retain the resulting backup directory.

Do not delete, overwrite, or manually edit your existing SQLite database.
For best isolation, run the first v1.1 migration trial using a second Windows
test user or a spare copy of the complete data folder, not production records.

## B. Build v1.1

In **Git Bash**:

```bash
cd /c/Users/borot/OneDrive/Desktop/CS4433-Moderated-LMS
git switch feature/phase8-cr1-class-extensions
git pull --ff-only origin feature/phase8-cr1-class-extensions
git rev-parse HEAD
```

In **MSYS2 UCRT64**:

```bash
cd /c/Users/borot/OneDrive/Desktop/CS4433-Moderated-LMS
cmake -S . -B /c/dev/cs4433-lms-v11 -G Ninja -DCMAKE_BUILD_TYPE=Debug -DLMS_ENABLE_COVERAGE=OFF
cmake --build /c/dev/cs4433-lms-v11
ctest --test-dir /c/dev/cs4433-lms-v11 --output-on-failure
```

Target: **128 tests passed, 0 failed**.

Only once the tests pass, close any old LMS instance and launch the v1.1
application:

```bash
/c/dev/cs4433-lms-v11/ModeratedLMS.exe
```

The first startup upgrades the selected Windows profile's database by adding
`class_deadline_extensions`. This changes the application schema from 12
to 13 tables but leaves the existing 12 tables and their data unchanged.
Keep the pre-upgrade v1.0 backup for rollback.

## C. Manual GUI confirmation — demonstration data

1. Sign in as an Administrator and prepare an **assigned Lecturer**,
   two enrolled Students, and a demonstration course/assignment.
2. Log in as Lecturer, open **Assignments and extensions**. The new
   **Whole-class deadline extension (Lecturer)** group should be visible.
3. Choose an assignment and a new later deadline. Click
   **Grant whole-class extension** and confirm the dialog.
4. Log in as Student A, refresh the assignment list and inspect
   **Original due**, **Class due**, and **Your due**. The class and personal
   due dates should reflect the new extension.
5. Log in as Student B, enrolled in the same course. The same new class
   deadline should appear for Student B.
6. As Lecturer, give Student A an individual extension that is even later
   than the class deadline. Student A's **Your due** must remain their
   longer individual date; Student B's date remains the class date.
7. Try to set the class due date to the same or an earlier date. Expect
   an error, and no deadline should be shortened.
8. Have a Student submit work, then the assigned Lecturer mark it.
   Try to grant another class-wide extension to that assignment. Expect
   the request to be rejected after marking started.
9. Check that existing Student submission versions remain unchanged,
   the latest effective deadline controls future resubmissions, and
   the HoD cannot moderate or release before an extended future deadline.
10. Close and reopen the LMS. Confirm the class extension persists.

Use **fictional accounts and assignments** for this first upgrade test.
Screenshots should show the class extension form, class due/personal due
columns, unchanged longer individual deadline, and the post-marking
rejection message.

## D. Regression and backward compatibility

- Original individual extension UI and validation must continue working.
- Original deadline and historical submission version timestamps remain
  unchanged.
- Late penalties are calculated from the resolved **current** effective
  deadline; immutable historical `is_late` flags may still reflect the
  deadline in force at the time a version was submitted.
- Backup creation and verification must work with the 13-table v1.1 database.
- The v1.0 code and release folder are retained separately; do not point a
  v1.0 executable at a migrated database for rollback. Restore the verified
  v1.0 snapshot offline before reopening v1.0.
- CI build/coverage status is tracked in the branch's GitHub Actions runs.

## E. Submission evidence

Record a commit SHA, the exact test count/result, screenshots, a migration
test log, any defects and retests, and a supervisor acceptance decision.
Only then consider merging/tagging the v1.1 release.
