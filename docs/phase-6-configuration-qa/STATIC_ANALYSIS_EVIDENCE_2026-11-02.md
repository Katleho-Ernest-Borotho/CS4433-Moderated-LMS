# Phase 6 — Static Analysis Evidence (2 November 2026)

## Tool and scope

**Tool:** cppcheck, run in Windows **MSYS2 UCRT64** from the
`CS4433-Moderated-LMS` repository root.

**Command executed:**

```bash
cppcheck --enable=warning,performance,portability \
  --std=c++17 \
  --suppress=missingIncludeSystem \
  -I include src 2>&1 | tee /c/dev/cs4433-lms-coverage/cppcheck-report.txt
```

## Actual observations from developer terminal

| Measurement | Observed value |
|---|---|
| Source files visited | **29 of 29** |
| Final progress | **100% done** |
| Reported `warning` diagnostics | **0** |
| Reported `performance` diagnostics | **0** |
| Reported `portability` diagnostics | **0** |
| Saved local text report | `/c/dev/cs4433-lms-coverage/cppcheck-report.txt` |
| Verification status | **No findings emitted within the selected cppcheck categories** |

Cppcheck analysed application, domain, persistence, security, tools, UI and
entry-point source code. This result is a clean **configured static-analysis
baseline**, not a guarantee of zero software defects. Because the command
piped output through `tee` without explicitly checking
`PIPESTATUS`, the final command exit status was not separately recorded.
The developer's output shows that the scan completed all 29 source files.

## Complementary verified evidence

- CTest: **115/115 tests passed**, **45.66 seconds** (coverage-enabled build).
- GCC/gcovr application + domain coverage: **484/524 lines**, **92%**.
- NFR-01: 800 Student accounts, **20/20 successful logins in under 2 seconds**.

## Remaining Phase 6/7 actions

1. Repeat cppcheck with `--error-exitcode=1` and capture a clear pass/fail
   command status to strengthen automation evidence.
2. Review `--enable=style` and `--enable=unusedFunction` findings separately;
   record false positives with evidence rather than indiscriminately ignoring
   warnings.
3. Run the manual ST-01..ST-16 system and acceptance tests, including
   keyboard-only navigation and offline operation, then record actual outcomes.
4. Package an isolated **Release** build with all Qt runtime and other UCRT64
   DLL dependencies, and test launch on a **second Windows laptop** without
   the developer's MSYS2 PATH.
5. Retain the compiler version, OS version, release build SHA, test screenshots,
   performance output, coverage report and supervisor Gate 3 decision.

This report records the supplied results but does not claim the deployment
and manual acceptance tests have already been performed.
