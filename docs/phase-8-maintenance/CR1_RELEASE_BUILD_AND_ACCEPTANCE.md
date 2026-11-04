# CR1 v1.1 — Release Packaging and Independent Windows Acceptance

**Package status:** v1.1 Release package built successfully. The developer
reported that the requested independent second-laptop tests worked.
Exact package: `ModeratedLMS_v1_1_20261102_023504`; built at
`2026-11-02T02:35:35Z` from commit
`adbd8e194fc0856e9e478880d385a1b1c14d0a5e`.

## Step 1 — Preserve v1.0 working state

On the development PC, using the tested v1.0 LMS:

1. Administrator → **Maintenance and reports** → **Create full backup**.
2. Select a private external folder; verify the completed backup.
3. Retain the old v1.0 package; do not overwrite or delete it.
4. Close all LMS windows before running a v1.1 program on the same profile.

For the safest acceptance run, use synthetic records and a **dedicated
Windows test profile**. Do not expose real learner information.

## Step 2 — Update branch in Git Bash

```bash
cd /c/Users/borot/OneDrive/Desktop/CS4433-Moderated-LMS
git fetch origin
git switch feature/phase8-cr1-class-extensions
git pull --ff-only origin feature/phase8-cr1-class-extensions
git rev-parse HEAD
```

Record the Git commit SHA alongside your packaging evidence.

## Step 3 — Package v1.1 in MSYS2 UCRT64

```bash
cd /c/Users/borot/OneDrive/Desktop/CS4433-Moderated-LMS
bash scripts/package_windows_ucrt64.sh
```

This invokes a separate CMake Release build under
`C:\dev\cs4433-lms-release-v11` and creates a new timestamped directory:

```text
C:\dev\cs4433-lms-packages\ModeratedLMS_v1_1_YYYYMMDD_HHMMSS
```

Expected package content: `ModeratedLMS.exe`, `ModeratedLMSRestore.exe`,
`lms_schema.sql`, Qt/UCRT64 DLLs and plugin folders,
`migration-reference/001_class_deadline_extensions.sql`,
`README_FIRST_RUN.txt` and `BUILD_PROVENANCE.txt`.

The DDL migration is embedded in the executable and runs automatically.
The `migration-reference` copy is **documentation only**; do not execute it
manually for an ordinary upgrade.

The package must **not** contain `lms.sqlite3`, school submission files,
test database credentials, the Git repository, or
`ModeratedLMSAdminRecovery.exe`. Check for their absence before sharing.

## Step 4 — Independent second-laptop test

Copy the **entire** generated directory (ZIP the directory if needed) to
the second Windows laptop. Extract it with Qt plugin subfolders unchanged.
Test with the LMS closed and without reliance on MSYS2, CMake or Qt Creator.

| Check | Procedure | Required observed result | Result / screenshot |
|---|---|---|---|
| V11-D01 | Run packaged `ModeratedLMS.exe` offline | No missing DLL/plugin error; GUI opens | **User-reported PASS** |
| V11-D02 | Use fresh test Windows profile | Initial Administrator setup works | |
| V11-D03 | Create Lecturer, two Students, course and assignment | Roles/course access work as v1.0 | |
| V11-D04 | Lecturer grants whole-class extension | New later class deadline saved | **User-reported PASS** |
| V11-D05 | Students A and B view assignment | Both see same Class due date | **User-reported PASS** |
| V11-D06 | Give A a longer individual extension | A's Your due stays later; B uses class date | **User-reported PASS** |
| V11-D07 | Lecturer proposes equal/shorter class deadline | Request rejected; existing dates unchanged | |
| V11-D08 | Lecturer marks a submission, then tries class extension | Extension rejected after marking | |
| V11-D09 | Close and reopen packaged v1.1 | Accounts and class extension persist | **User-reported PASS** |
| V11-D10 | Administrator creates and verifies full backup | New 13-table SQLite snapshot and attachments preserved | **Backup verification user-reported PASS; table/attachment details not separately supplied** |
| V11-D11 | Restore safely with LMS closed using demo data | Selected backup, including CR1 deadline, restored | |
| V11-D12 | Use second profile containing backed-up v1.0 demo data | First launch adds 13th table, preserves old 12 tables/data | |

Do not check off a test unless its step was performed. In particular,
the second-laptop launch of v1.0 is **not evidence** that V11-D01 passed.

## Step 5 — Negative/recovery checks

- With normal LMS running, the Restore tool must refuse to open the database.
- Do not attempt a downgrade of the *upgraded* SQLite file by opening v1.0.
  For rollback: exit v1.1, restore the verified v1.0 database+attachments,
  then start the original v1.0 package.
- If the Qt Windows plugin cannot load, verify `platforms/qwindows.dll`
  and the matching Qt libraries in the extracted folder.
- If a backup cannot verify, **do not restore** from it.
- Review third-party redistribution and licensing notices before release.

## Step 6 — Release evidence and decision

| Evidence item | Required entry |
|---|---|
| Package folder | `C:\\dev\\cs4433-lms-packages\\ModeratedLMS_v1_1_20261102_023504` |
| Build timestamp | `2026-11-02T02:35:35Z` |
| Git SHA (BUILD_PROVENANCE.txt) | `adbd8e194fc0856e9e478880d385a1b1c14d0a5e` |
| Windows edition/architecture on second PC | |
| V11-D01..V11-D12 pass/fail | Requested second-PC checks user-reported PASS; D02/D03/D07/D08/D11/D12 require individually recorded results |
| Observed CTest / CI 128/128 status | Already PASS for implementation checkpoint |
| Changed/missing files or defects | |
| Backup folder for rollback | |
| Supervisor approval | |

**Do not tag/merge a final v1.1 release until independent deployment and
backup/restore verification are recorded.** A successful package construction
is not the same as successful second-computer acceptance.

## Follow-up user confirmation and evidence limits

The developer provided the complete `BUILD_PROVENANCE.txt` output and
confirmed **"everything worked"** in direct reply to the requested
offline launch, Lecturer class extension, Student class/personal deadline,
restart persistence, and backup-verification checks. The observations
above are marked **user-reported PASS** and should be paired with
second-laptop screenshots in the final evidence package.

The additional negative tests, existing-v1.0-profile upgrade,
actual offline **restore**, and licensing/supervisor sign-off have not
been individually evidenced by this reply; leave them unmarked until
performed. Automated unit/regression coverage is already 128/128 PASS.
