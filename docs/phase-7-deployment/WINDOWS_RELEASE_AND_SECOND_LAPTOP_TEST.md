# Phase 7 — Windows Deployment and Second-Laptop Verification

## 1. Target setup

The v1.0 Moderated LMS is an **offline, single-computer Windows desktop**
application. This acceptance package uses GCC/MinGW-w64 **UCRT64**, Qt 6 Widgets,
SQLite and libsodium. The source development computer is not the deployment
target: confirm launch and operation on the **second Windows laptop** without
the developer's MSYS2 PATH.

**Important:** The release package has not yet been built or executed on the
second laptop. These steps are the planned acceptance procedure.

## 2. Make a clean deployable folder on the development PC

Open **Git Bash** for the source update:

```bash
cd /c/Users/borot/OneDrive/Desktop/CS4433-Moderated-LMS
git switch feature/phase4-implementation
git pull --ff-only origin feature/phase4-implementation
git rev-parse HEAD
```

Then open **MSYS2 UCRT64**:

```bash
cd /c/Users/borot/OneDrive/Desktop/CS4433-Moderated-LMS
bash scripts/package_windows_ucrt64.sh
```

The script uses a separate Release build directory:
`C:\dev\cs4433-lms-release`. It creates a timestamped folder under
`C:\dev\cs4433-lms-packages`. It compiles only
`ModeratedLMS.exe` and `ModeratedLMSRestore.exe`, and packages
`lms_schema.sql`.

The script also invokes MSYS2's `windeployqt6` to copy Qt runtime/plugins,
then scans the DLL import lists and copies non-Qt dependencies from
`C:\msys64\ucrt64\bin`. The separate
`ModeratedLMSAdminRecovery.exe`, test binaries, repository history, the
real SQLite database and the attachment storage directory are not packaged.

Qt's deployment tooling does **not** automatically cover every third-party
library, hence the separate dependency scan. The script requires review of
the collected files, redistribution licences and independent launch.

**Keep these files together:** the two EXEs, `lms_schema.sql`, root-level
DLLs, Qt `platforms/` and other plugin folders. Do not flatten plugin paths.

## 3. Move the package

Copy the entire timestamped `ModeratedLMS_v1_0_...` folder to a USB drive
or compress that *folder* as ZIP using Windows Explorer. Transfer it to the
second Windows laptop, then extract it. **Do not transfer live school data**
or your development account's private SQLite directory.

Use a temporary, fictional Administrator and demonstration users on the
second laptop. Do not assume the second machine shares the first machine's
accounts.

## 4. Second-laptop acceptance checks

| ID | Check | Expected | Actual / evidence |
|---|---|---|---|
| D-01 | Open `ModeratedLMS.exe` by double-click, with MSYS2 not installed | GUI opens without DLL/plugin errors | |
| D-02 | Check app directory includes `lms_schema.sql` | Initial schema loads; no missing-file error | |
| D-03 | Fresh Windows account/local app data | Initial Administrator setup shown; no default password | |
| D-04 | Create a test Admin/Lecturer/Student/HoD, course and enrolment | Roles and records work without Internet | |
| D-05 | Student submits a test TXT/PDF and logs in again | Submission and attachment survive restart | |
| D-06 | Lecturer marks a result, HoD approves/releases after deadline | Marking and release rules hold | |
| D-07 | Student views officially released result | Own mark and feedback visible; unreleased hidden | |
| D-08 | Administrator creates and verifies a full backup | Database plus attachments present in protected backup folder | |
| D-09 | Run `ModeratedLMSRestore.exe` while LMS open | Restore refuses due to application lock | |
| D-10 | Close LMS and restore a synthetic backup | Previous test records and attachments return | |
| D-11 | Disconnect all network adapters and repeat login/submit/mark | No network service required | |
| D-12 | Use keyboard-only Tab/Shift+Tab/Enter/Space to complete key workflows | Primary controls accessible; issues logged | |

For each test save a screenshot, the date, result, Windows edition, build SHA
and any defect ID. Also record missing DLL names if Windows reports them.

Do not copy or hand-edit `lms.sqlite3` while the LMS is open.
A normal first launch creates its data directory using
`QStandardPaths::AppLocalDataLocation` for
`CS4433/ModeratedLMS`, and loads the schema from the EXE directory.

## 5. Handling failure

- **Application does not start / missing DLL:** Record the *exact* DLL name
  and compare the source deployment folder with the second laptop's copy.
  Inspect runtime dependencies in the MSYS2 UCRT64 build; do not blindly
  copy random DLLs from another toolkit.
- **Qt platform plugin error:** Verify that `qwindows.dll` remains inside
  the correct Qt `platforms` plugin subfolder, and the Qt DLL versions match.
- **No schema:** Restore `lms_schema.sql` next to the EXE from the
  deployment package; never replace the user's existing database.
- **Different data after transferring:** This is expected. The app stores
  data in per-user Windows Local AppData, not beside the EXE. For a clean
  deployment test, keep synthetic data only.

## 6. Deployment completion gate

Deployment can be marked PASS only after the second-laptop checks have been
executed and their results captured. Before any real-user deployment, review
redistribution obligations and include Qt and third-party licence notices,
application backup safeguards, user training, rollback instructions and
supervisor acceptance. This folder is **not** a signed installer.
