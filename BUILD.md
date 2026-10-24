# Build and test the Moderated LMS

The Windows development environment uses MSYS2 UCRT64, C++17, Qt 6 Widgets, SQLite, libsodium, CMake, Ninja and GoogleTest.

## Update source code

Run in **Git Bash** (where GitHub browser authentication is configured):

```bash
cd /c/Users/borot/OneDrive/Desktop/CS4433-Moderated-LMS
git switch feature/phase4-implementation
git pull --ff-only origin feature/phase4-implementation
```

## Build, test and run

Run in **MSYS2 UCRT64**, from the repository directory:

```bash
cd /c/Users/borot/OneDrive/Desktop/CS4433-Moderated-LMS
cmake -S . -B /c/dev/cs4433-lms-build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build /c/dev/cs4433-lms-build
ctest --test-dir /c/dev/cs4433-lms-build --output-on-failure
/c/dev/cs4433-lms-build/ModeratedLMS.exe
```

Do not paste terminal prompts or output lines into the shell. The Qt Creator IDE can also open `CMakeLists.txt` with the `CS4433 Qt 6 UCRT64` kit.

## Persistence and first launch

The program stores the local `lms.sqlite3` in the Windows application-data folder for `CS4433/ModeratedLMS` and uses the approved twelve-table Week 3 SQLite schema. Existing users and enrolments persist between launches. Rebuilding the project does not reset the database.

If there are no users, create an administrator. The program contains no default passwords. Usernames must contain 3–40 ASCII letters/digits/dots/dashes/underscores, and initial passwords must be at least 12 UTF-8 bytes. The administrator creates ordinary accounts on the User Management tab; only salted hashes are persisted.

## New academic management workflow

After logging in as an Administrator:

1. Open **Academic management**.
2. Create a department (for example, `Computer Science`).
3. Create a course associated with the department (for example, `CS4433` / `Software Engineering`).
4. In the **User management** tab, create an active Student account and an active Lecturer account if needed.
5. Return to **Academic management**, press **Refresh departments, courses and users** when needed, then enrol the student and assign the lecturer to the course. Optional: create a HoD account and assign the HoD to the department.
6. Log out, log in as the Student, and verify only enrolled courses appear. Log in as the Lecturer to verify only assigned courses appear. A HoD only sees courses for the HoD's assigned department.

Administrators can rename a department and edit a course title. Changes and allocations persist in SQLite. Duplicate course codes, duplicate department names, duplicate allocation records and wrong-role assignments are rejected. Only active users of the appropriate roles can be allocated. Service-layer authorisation prevents students from using administrator operations even when GUI controls are not shown.

## Automated tests and scope

The GoogleTest suite contains 28 test cases, including database creation/persistence, authentication, user management, department/course CRUD, role-based allocations, and role-scoped course visibility. Run CTest after each update and share the error output if the build or tests fail.

This is **not** the full LMS yet. The assignment creation screen, extensions, typed/file submissions, resubmission, penalty calculation, marking, moderation, release and mark-change audits are planned for later implementation. Account update/password reset and some additional administrative editing controls are also pending. Do not use real student records until backup and security review are complete.
