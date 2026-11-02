# Phase 8 — v1.1 CR1 Regression Test Evidence (2 November 2026)

## Verified local build and test results

The developer supplied MSYS2 UCRT64 terminal output on 2 November 2026
from `C:/Users/borot/OneDrive/Desktop/CS4433-Moderated-LMS` showing:

```text
cmake -S . -B /c/dev/cs4433-lms-v11 -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build /c/dev/cs4433-lms-v11
ctest --test-dir /c/dev/cs4433-lms-v11 --output-on-failure
[41/41] Linking CXX executable lms_tests.exe
100% tests passed out of 128
Total Test time (real) = 53.26 sec
```

| Measure | Result |
|---|---|
| Platform | Windows, MSYS2 UCRT64, GNU C++ 16.2.0 |
| SQLite | 3.54.0 |
| libsodium | 1.0.22 |
| CMake configuration | PASS |
| Compilation | PASS (41/41) |
| Automated tests run | **128** |
| Passed | **128** |
| Failed | **0** |
| CTest elapsed time | **53.26 s** |
| CR1 regression tests included | **13** |
| Local test conclusion | **PASS** |

The passing test names in the supplied output include:
`DatabaseTests.MigratesV10DatabaseWithoutLosingAccountsOrRepeatingChanges`,
`AssignmentAndExtensionTests.ClassExtensionChangesDueForAllEnrolledStudents`,
`AssignmentAndExtensionTests.LongerIndividualExtensionSurvivesClassExtension`,
`AssignmentAndExtensionTests.ClassExtensionWinsOverShorterIndividualExtension`,
`SubmissionTests.ClassExtensionReopensResubmissionForWholeClass`,
`MarkingTests.ClassExtensionRecalculatesPenaltyForPreexistingSubmission`,
`MarkingTests.CannotRetroactivelyExtendClassAfterMarking`, and
`ModerationTests.ClassExtensionBlocksPrematureHoDModeration`.

The original 115 v1.0 tests continued to pass in the same test suite.
The developer's terminal log does not print `git rev-parse HEAD`, so exact
local source revision remains to be recorded alongside the terminal log.

## Independent CI verification

GitHub Actions successfully completed its Windows UCRT64 build/test/coverage
workflow for source commit
`57a1e2d39725d97c1fdbbd597f4afe67d0057ecb`:

https://github.com/Katleho-Ernest-Borotho/CS4433-Moderated-LMS/actions/runs/41376420548

The CI run status was checked from GitHub and reported `success`. The
exact v1.1 line coverage percentage has not been extracted from its report,
so the earlier v1.0 coverage measurement should not be represented as v1.1
coverage without an updated measurement.

## Acceptance still outstanding

Automated passing tests are not evidence that the new v1.1 GUI control
is understandable or that the lecturer/student views work after upgrading
an existing deployment. Next retain the **verified full v1.0 backup** and
perform the manual GUI scenarios in
`CR1_BUILD_AND_GUI_VERIFICATION.md`:

- Lecturer grants a whole-class extension and sees the displayed new date.
- Two enrolled Students observe the new Class due deadline.
- One Student's longer individual deadline is not shortened.
- Equal/earlier proposed class deadlines are rejected.
- New class extensions after marking starts are rejected.
- The deadline change survives application restart.
- Existing users, courses, results and submission versions remain intact.
- Backup/restore with a v1.1 (13-table) database is confirmed.

**CR1 state:** Automated integration/regression **PASS**. Manual GUI and
post-migration acceptance **pending**. Do not merge/tag a released v1.1
solely from the automated pass.
