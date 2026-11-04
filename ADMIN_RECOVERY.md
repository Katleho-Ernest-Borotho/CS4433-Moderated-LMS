# Recovering an Administrator password (local development only)

The Moderated LMS uses salted libsodium password hashes. An old password cannot be
viewed or retrieved. **Do not remove, overwrite or manually recreate the database.**

## For a Student, Lecturer, or HoD account

Sign in as Administrator. Open **User management**, select the account,
choose **Reset password**, and enter a new temporary password (12–1024 UTF-8
bytes) twice. The old password and previously issued in-app sessions become
invalid. The role, user ID and academic records remain the same.

## For a forgotten Administrator account

If another Administrator still works, sign in as that account and reset the
first Administrator through User management.

If every Administrator account is inaccessible, use the optional
**local development recovery executable** on the same Windows account that
runs the LMS. You must have authorised access to this computer and its files.

1. Close every running **Moderated LMS** window. Do not run the main LMS while
   using recovery.
2. In **Git Bash**, update your source:

   ```bash
   cd /c/Users/borot/OneDrive/Desktop/CS4433-Moderated-LMS
   git switch feature/phase4-implementation
   git pull --ff-only origin feature/phase4-implementation
   ```

3. In **MSYS2 UCRT64**, enable this optional build target and compile:

   ```bash
   cd /c/Users/borot/OneDrive/Desktop/CS4433-Moderated-LMS
   cmake -S . -B /c/dev/cs4433-lms-build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DLMS_ENABLE_LOCAL_ADMIN_RECOVERY=ON
   cmake --build /c/dev/cs4433-lms-build --target ModeratedLMSAdminRecovery
   /c/dev/cs4433-lms-build/ModeratedLMSAdminRecovery.exe
   ```

4. Read the security confirmation, select the Administrator account, enter
   and confirm a **new** password (minimum 12 characters), then approve the
   reset. There is no built-in default recovery password.
5. The utility creates a dated SQLite database backup in the same application
   data folder **before** changing the hash. It does not delete users, assignments,
   versions, results or other academic records. If the Administrator was inactive,
   recovery also reactivates that selected account.
6. Close recovery and start the ordinary LMS with
   `/c/dev/cs4433-lms-build/ModeratedLMS.exe`. Log in using the selected
   Administrator username and new password. Confirm that all existing user
   records and courses are still present.
7. Disable the recovery build flag and delete the optional recovery executable
   when finished:

   ```bash
   cmake -S . -B /c/dev/cs4433-lms-build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DLMS_ENABLE_LOCAL_ADMIN_RECOVERY=OFF
   cmake --build /c/dev/cs4433-lms-build
   rm -f /c/dev/cs4433-lms-build/ModeratedLMSAdminRecovery.exe
   ```

### Security notes

- This tool is **disabled by default** in CMake and is for the authorised
  owner/developer of the single-computer offline LMS. **Do not distribute the
  recovery executable to school users.** It grants powerful local access.
- It requires access to the real existing database inside the current
  Windows user's local application data. It does not create a new database.
- The SQLite online backup API is used to create a consistent snapshot.
  All password changes are made through the existing transactional repository
  using prepared queries and fresh Argon2id/libsodium password hashes.
- Backups contain user records, password hashes, submission text and marks.
  Keep them protected, and remember to copy the local `attachments` directory
  when making a full disaster-recovery backup.
- This executable is a recovery mechanism for a trusted development machine,
  **not** a substitute for an identity-verified production recovery workflow.
- The new executable still needs a successful build and manual test on the
  target Windows/MSYS2 UCRT64 environment.
