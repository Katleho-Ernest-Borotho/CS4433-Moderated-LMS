# Build and run the Moderated LMS

This project uses C++17, Qt 6 Widgets, SQLite, libsodium, CMake, Ninja and GoogleTest. Use **MSYS2 UCRT64** on Windows.

From the repository directory:

```bash
cmake -S . -B /c/dev/cs4433-lms-build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build /c/dev/cs4433-lms-build
ctest --test-dir /c/dev/cs4433-lms-build --output-on-failure
/c/dev/cs4433-lms-build/ModeratedLMS.exe
```

Alternatively, open `CMakeLists.txt` in Qt Creator with the `CS4433 Qt 6 UCRT64` kit. Use an out-of-source build directory outside OneDrive.

## First launch

The program creates `lms.sqlite3` in the Windows local application-data directory for CS4433/ModeratedLMS, then installs the unchanged 12-table Week 3 schema. If no users exist, it displays an initial administrator setup form. Choose your own username (3-40 letters, digits, dots, dashes or underscores) and password (at least 12 UTF-8 bytes). Re-enter the password to confirm. There are **no hard-coded or default credentials**. The account stores only a salted libsodium password hash.

After creating the administrator, log in using those credentials. A signed-in administrator sees a role-labelled placeholder screen and can log out. Student, Lecturer and Head of Department accounts and their functional dashboards will be implemented in later work; do not mistake the placeholder for a complete LMS.

## Security and testing

The GUI sends credentials to `AuthenticationService`, which depends on `IUserRepository` and `IPasswordHasher`. `SQLiteRecordRepository` performs prepared SQLite queries. The account can be bootstrapped only while there are no users, checked inside an immediate SQL transaction. Inactive accounts and incorrect credentials are refused. Services re-check an account before performing role-restricted actions. Passwords are never stored as plaintext in the database.

The CTest suite includes 5 initial tests plus 8 authentication tests: missing credentials, wrong password, protected roles, inactive account/session, duplicate setup, credential validation and SQL-injection-style input.

This is an **implementation checkpoint**, not a finished product. Academic administration, course authorisation, assignments, submissions, marking, moderation and release remain to be built and tested. Do not use real student records until those functions and the final security review are complete.
