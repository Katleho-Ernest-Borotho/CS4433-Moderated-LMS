# Phase 5 — Verification evidence record (2026-11-02)

## Locally executed GoogleTest / CTest suite

Source of evidence: Windows **MSYS2 UCRT64** terminal output supplied
from `K4T7-Borotho`, in the project directory
`C:/Users/borot/OneDrive/Desktop/CS4433-Moderated-LMS`.

| Field | Recorded result |
|---|---|
| Execution date | 2 November 2026 (date of received terminal output) |
| Build generator | Ninja |
| Build mode | Debug |
| Commands | `cmake -S . -B /c/dev/cs4433-lms-build -G Ninja -DCMAKE_BUILD_TYPE=Debug`; `cmake --build /c/dev/cs4433-lms-build`; `ctest --test-dir /c/dev/cs4433-lms-build --output-on-failure` |
| CTest cases executed | **115** |
| Passed | **115** |
| Failed | **0** |
| CTest summary | **100% tests passed out of 115** |
| Total wall time reported by CTest | **46.10 seconds** |
| Source commit SHA | **Not printed in the supplied terminal output; must be captured with `git rev-parse HEAD` for strict provenance** |
| Observed warning | Repeated CMake author warning that `SQLite::SQLite3` is deprecated |
| Follow-up code change | Changed `CMakeLists.txt` to use `SQLite3::SQLite3`; requires reconfigure to confirm warning is gone |

The passing cases include the cross-role Student → Lecturer → HoD →
Student release integration and authorisation/audit tests, along with
backup/restore and account operations.

**Interpretation:** The provided output establishes successful local automated
execution of the 115 cases. It does **not** establish measured line coverage,
a passing user-acceptance session, an 800-account performance benchmark,
keyboard-accessibility compliance, or the Phase 5 Gate 3 decision.

## Continuous integration

A GitHub Actions Windows UCRT64 workflow run for commit
`0e5d9c68d8484a7ebe274ef2373623b416d1245c` completed with status
**success**, independently checked from the workflow-run status:

https://github.com/Katleho-Ernest-Borotho/CS4433-Moderated-LMS/actions/runs/41369413404

This confirms the workflow completed successfully for that commit. The actual
business-logic line coverage percentage must still be read from
`business-coverage.xml` / the coverage HTML artifact. A successful workflow
run alone does not show that the course's 70% line-coverage target was met.

## Evidence still to capture

- `git rev-parse HEAD` and a screenshot or saved output of the local CTest run
  to associate it with one immutable source revision.
- The actual gcovr business-logic coverage percentage and scope.
- NFR-01: results of 20 measured logins with 800 stored Student accounts.
- Manual system test outcomes ST-01..ST-16 including exception-path evidence.
- Offline NFR-02, password-database inspection NFR-03, usability NFR-04,
  keyboard navigation NFR-05 and 20-record restart persistence NFR-06.
- Static-analysis findings, fixes and rerun, and signed Gate 3 review.

The current checkpoint follows the submitted requirements baseline and does
not claim those remaining measurements were already performed.
