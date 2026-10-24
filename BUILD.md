# Building and Testing the Moderated LMS

Windows development: MSYS2 UCRT64, C++17, Qt 6 Widgets, SQLite, libsodium, CMake, Ninja, GoogleTest.

## Update the project in Git Bash

Close the running LMS before rebuilding.

```bash
cd /c/Users/borot/OneDrive/Desktop/CS4433-Moderated-LMS
git switch feature/phase4-implementation
git pull --ff-only origin feature/phase4-implementation
```

## Build, test and run in MSYS2 UCRT64

```bash
cd /c/Users/borot/OneDrive/Desktop/CS4433-Moderated-LMS
cmake -S . -B /c/dev/cs4433-lms-build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build /c/dev/cs4433-lms-build
ctest --test-dir /c/dev/cs4433-lms-build --output-on-failure
/c/dev/cs4433-lms-build/ModeratedLMS.exe
```

Alternatively, open CMakeLists.txt from Qt Creator using the CS4433 Qt 6 UCRT64 kit. Use a build folder outside the OneDrive repository.

## Local storage

The program keeps its database under the Windows local application-data directory for CS4433/ModeratedLMS. Updating the program does **not** remove accounts, courses, enrolments or existing assignments. The approved twelve-table SQLite schema is unchanged in this checkpoint. The application is offline.

## Checkpoint walkthrough — two new features together

### Feature A: Lecturer creates assignments (FR-08)

1. Sign in as Administrator. In **User management**, create a temporary Lecturer account if none exists.
2. In **Academic management**, assign this lecturer to the existing CS4433 course. The student must already be enrolled in that course.
3. Log out and log in as the Lecturer.
4. Open **Assignments and extensions**, choose CS4433.
5. Fill in a title such as "Lab 1", select the deadline using local date/time, set a maximum mark such as 100, and choose **Create assignment**.
6. Confirm that the new assignment appears in the list. The chosen time is stored in SQLite as UTC; the application uses the system's local time zone in its date editors.

### Feature B: Individual deadline extension (FR-09)

1. While logged in as that Lecturer, choose the newly created assignment and the enrolled Student.
2. Observe the student's current effective deadline.
3. Choose a later new deadline and select **Grant individual extension**.
4. Confirm that the effective deadline changes. Extensions are attached to one Student and one Assignment; no whole-class extension is created.
5. Log out and log in as that Student. Open **Assignments and extensions**. The Student should see the assignment with both the original deadline and the individual effective deadline.
6. Check that another unenrolled student cannot see the assignment and that an unassigned lecturer cannot edit it.

Repeat these tests after restarting the application to verify persistence.

## Security and business rules

The GUI calls application services rather than using SQL. An authenticated Lecturer may create assignments and grant extensions only for courses assigned to that Lecturer. A grant applies only to an active, enrolled Student. An extension must be later than the student's current effective deadline; it cannot shorten the deadline. Inputs use canonical UTC YYYY-MM-DDTHH:MM:SSZ format and validation checks the calendar date. The maximum mark must be positive and finite.

Students can read only assignments in courses they are enrolled in, and can see only their own effective deadlines. The SQLite layer uses parameterised queries and transactions. There are now 39 registered GoogleTest cases covering previous features and the new assignment/extension permissions and boundaries.

## Implementation status

The two new features and tests have been committed, but they still require a successful build and test run on the development laptop. Submissions, resubmission rules, late penalties, marking, moderation, mark-change audit and release of results are **not** implemented yet. Do not use real student information before backup, recovery and full security testing are complete.
