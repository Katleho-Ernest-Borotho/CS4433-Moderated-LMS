# Build and test the Moderated LMS

The Windows development environment uses MSYS2 UCRT64, C++17, Qt 6 Widgets, SQLite, libsodium, CMake, Ninja and GoogleTest.

## Update source code

Run in **Git Bash** (if Git authentication is configured there):

```bash
cd /c/Users/borot/OneDrive/Desktop/CS4433-Moderated-LMS
git switch feature/phase4-implementation
git pull --ff-only origin feature/phase4-implementation
```

## Build, test, and run

Run in **MSYS2 UCRT64**, from the repository directory:

```bash
cd /c/Users/borot/OneDrive/Desktop/CS4433-Moderated-LMS
cmake -S . -B /c/dev/cs4433-lms-build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build /c/dev/cs4433-lms-build
ctest --test-dir /c/dev/cs4433-lms-build --output-on-failure
/c/dev/cs4433-lms-build/ModeratedLMS.exe
```

Alternatively, open `CMakeLists.txt` in Qt Creator with the `CS4433 Qt 6 UCRT64` kit, using a separate out-of-source build folder.

## Application walkthrough

On first launch, the application creates `lms.sqlite3` inside the Windows local application-data directory for `CS4433/ModeratedLMS` and initialises the twelve-table Week 3 database schema. It does **not** reset or overwrite existing users after an update.

If no user exists, create the first administrator with a unique username and password. Usernames must contain 3 to 40 ASCII letters/digits/dots/dashes/underscores. Passwords must contain 12 to 1024 UTF-8 bytes. Enter your own credentials; the program has no default password.

Log in as the administrator. You can now:

- Create Student, Lecturer, Head of Department or additional Administrator accounts.
- Enter and confirm each user's initial password.
- View account IDs, usernames, roles and active/inactive status.
- Refresh the list of users.
- Select an account and deactivate it after confirming the action.
- Log out, and test logging in as an ordinary account.

Ordinary users do not see the administrative user-management panel, and the service enforces this restriction even if the GUI is bypassed. The application prevents administrator self-deactivation and deactivation of the last active administrator. Deactivated users cannot log in. Account records remain in SQLite rather than being deleted.

Do **not** share real passwords in screenshots or messages. Use temporary demonstration accounts only until backup, password reset and other production requirements have been implemented.

## Security and tests

The GUI calls `AcademicManagementService` and `AuthenticationService`, which use `IUserRepository` and `IPasswordHasher`. `SQLiteRecordRepository` performs parameterised SQLite operations and wraps writes in transactions. Only salted password hashes are persisted, and the user-list API never exposes them.

There are 19 GoogleTest test cases covering the initial domain and schema, first administrator setup, login, roles, invalid credentials, inactive accounts, SQL-injection-style input, user creation, duplicate usernames and account deactivation. Run CTest to verify the suite on the development machine.

The administrator user-management screen is a **checkpoint**, not a completed LMS. User editing, password reset, departmental assignment, enrolment, course management, submissions, marking, moderation and final result release remain to be implemented and tested.
