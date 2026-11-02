# Phase 5 — Verified Coverage and CTest Evidence (2026-11-02)

Environment: local Windows MSYS2 UCRT64; GCC coverage-enabled Debug build.

## Automated tests

- `ctest --test-dir /c/dev/cs4433-lms-coverage --output-on-failure`
- Passed: **115/115**; failed: **0**.
- Total execution time: **45.66 seconds**.

## Business-logic line coverage

- Tool: gcovr, report filters `src/domain/` and `src/application/`.
- Total measured executable lines: **524**.
- Lines executed: **484**.
- Coverage: **92%** as reported by gcovr.
- Course benchmark: **70%** line coverage. **PASS**.

| Source file | Coverage |
|---|---:|
| AcademicManagementService.cpp | 88% |
| AdminReportingService.cpp | 98% |
| AuthenticationService.cpp | 100% |
| BootstrapService.cpp | 100% |
| MarkingService.cpp | 97% |
| ModerationService.cpp | 95% |
| SubmissionManagementService.cpp | 82% |
| LatePenaltyPolicy.cpp | 100% |
| User.cpp | 100% |

Scope: these figures exclude `src/persistence` and GUI code. They do not substitute for manual usability, deployment or security testing.

Original terminal log supplied on 2026-11-02; exact tested commit was not printed. Record `git rev-parse HEAD` and retain the generated HTML report before the final submission.