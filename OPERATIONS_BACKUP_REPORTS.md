# Phase 4 — Three-feature operational checkpoint

This batch adds **full offline LMS backup**, **offline backup restoration**, and
**Administrator administrative reporting / CSV export** without changing the
approved twelve-table database schema.

## Update and build (Windows)

Close the running LMS. In **Git Bash**:

```bash
cd /c/Users/borot/OneDrive/Desktop/CS4433-Moderated-LMS
git switch feature/phase4-implementation
git pull --ff-only origin feature/phase4-implementation
```

Then in **MSYS2 UCRT64**:

```bash
cd /c/Users/borot/OneDrive/Desktop/CS4433-Moderated-LMS
cmake -S . -B /c/dev/cs4433-lms-build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build /c/dev/cs4433-lms-build
ctest --test-dir /c/dev/cs4433-lms-build --output-on-failure
/c/dev/cs4433-lms-build/ModeratedLMS.exe
```

The project currently contains **113 GoogleTest cases**. This is the expected
number until the development laptop reports successful compilation and CTest.

## Feature 1 — Back up all LMS data

1. Sign in as an **Administrator** and open **Maintenance and reports**.
2. Click **Create full backup...**.
3. Choose a separate, access-controlled folder (for example a local `C:\LMS-Backups`).
   Do not select a folder inside the active LMS application data.
4. Confirm. The application creates a uniquely named
   `ModeratedLMS_Backup_...` folder, then verifies the result.
5. Open the new folder in File Explorer. It should contain:
   - `lms.sqlite3` — SQLite database, including accounts, enrolments, mark and audit records
   - `attachments/files/` — stored PDF/DOCX/TXT submission files
   - `manifest.txt` — file list, byte sizes, SHA-256 checksums
6. Back in the maintenance tab, click **Verify backup folder...** and choose
   the created `ModeratedLMS_Backup_...` folder. Expect confirmation of a
   valid database and attachment count.

A backup is verified using stored SHA-256 hashes, SQLite `integrity_check`,
foreign-key check, the approved twelve-table schema and checks that every
referenced attachment exists. Files are copied using SQLite's online backup
API for the database. To avoid an inconsistent live snapshot, complete any
submission/marking operation before starting the backup.

**Privacy:** A backup contains student content, results and password hashes,
and is **not encrypted**. Keep it private and do not publish it in GitHub,
share it through public links or put real-student data in a demo backup.

## Feature 2 — Restore a saved LMS backup

**Important:** Restoration REPLACES the current local database and
submission files with the older backup's state. For safe testing, first create
a backup of your CURRENT dataset and use demonstration accounts only.

1. Record a test value that is easy to recognise (such as a course title or a
   demonstration account), then make a full backup in Feature 1.
2. Change that test value *after* making the backup. Refresh to confirm the new value.
3. **Close all regular LMS windows**.
4. In MSYS2 UCRT64 run:

   ```bash
   /c/dev/cs4433-lms-build/ModeratedLMSRestore.exe
   ```

5. Confirm the warning and select the backup folder. The tool validates the
   manifest, database and attachments **before** touching current files.
6. Type exactly `RESTORE` into the confirmation window.
7. The tool creates another backup of your **currently live** data in
   `ModeratedLMS_PreRestore_Safety` next to the active application-data folder,
   stages the selected backup, then swaps in its database and attachments.
   If staging or integrity verification fails, it aborts without replacement.
8. Restart the regular LMS. Check that the test value and stored attachment
   have returned to the backup's earlier state. Existing logins must use the
   credentials from the **restored snapshot**.
9. To undo the test restore, close the LMS and restore from the safety-backup
   folder produced in Step 7.

The running LMS and restore tool use the same `lms.lock`. Restore should
refuse to start while the regular LMS is active. It never performs a partial
in-app database overwrite.

## Feature 3 — Administrative reporting and CSV export

1. Log in as Administrator. Open **Maintenance and reports** and press
   **Refresh report**.
2. Verify the dashboard displays counts for active/inactive users,
   departments, courses, assignments, submissions, versions and provisional/
   approved/unreleased/released results.
3. Check the course table for `CS4433` and compare its enrolment/assignment/
   submission counts with the existing Academic management screen.
4. Click **Export administrative CSV...** and save the file as
   `ModeratedLMS_AdminReport.csv`.
5. Open the CSV using Excel or a text editor. The first section contains the
   aggregate counts; rows starting with `Course` contain course-level totals.
6. The CSV must **not** include account password hashes, passwords, individual
   submitted text or private provisional marks.

Only an authenticated Administrator can generate the report or launch a
backup through the maintenance screen. The app's reports are an aggregate
operational overview, **not** a transcript of individual student marks.

## Automated coverage and limitations

The source test suite has **113 cases**, including backup verification,
attachment copying, changed-dataset restoration, tampered backup rejection,
out-of-directory backups, reporting authorisation and CSV safety.

This code is newly committed. It has **not yet been compiled or run on the
Windows development machine**, so share the **first build error** or the
CTest result before treating this checkpoint as verified.

Limitations to address before production: backup encryption, scheduling/
retention, robust power-loss testing across filesystem replacement steps,
Windows account permission checks, restore on a clean second computer and
final audit/security evaluation. The backup is a folder-based package, not
a ZIP archive, and must be kept intact.
