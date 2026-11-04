# CS4433 — Measured C++ Source Metrics

Generated at (UTC): 2026-11-02 06:14:49
Git commit: `6854bc21cf01842816729e4d6232a8f4f8f02f68`

## Files and lines by code layer

| Layer | Source/header files | Physical lines | Approx. noncomment nonblank lines |
|---|---:|---:|---:|
| `src/application` | 7 | 801 | 731 |
| `src/domain` | 2 | 84 | 71 |
| `src/persistence` | 10 | 2191 | 2031 |
| `src/security` | 1 | 42 | 36 |
| `src/ui` | 6 | 2078 | 1946 |
| `src/tools` | 2 | 264 | 248 |
| `src` | 1 | 66 | 59 |
| `include` | 39 | 1305 | 1191 |
| `tests` | 2 | 2088 | 1914 |

**Totals:** 70 files, 8919 physical lines. 

## Ten longest source files

| File | Physical lines | Nonblank lines | Approx. code lines |
|---|---:|---:|---:|
| `tests/core_tests.cpp` | 2022 | 1859 | 1854 |
| `src/ui/LoginWindow.cpp` | 554 | 515 | 515 |
| `src/ui/AssignmentPanel.cpp` | 479 | 450 | 448 |
| `src/persistence/SQLiteAssignmentsRepository.cpp` | 314 | 287 | 285 |
| `src/ui/ResultsPanel.cpp` | 299 | 283 | 283 |
| `src/ui/MarkingPanel.cpp` | 292 | 275 | 273 |
| `src/persistence/SQLiteRecordRepository.cpp` | 285 | 262 | 262 |
| `src/ui/AcademicPanel.cpp` | 277 | 253 | 253 |
| `src/persistence/SQLiteAcademicRepository.cpp` | 274 | 248 | 248 |
| `src/persistence/SQLiteModerationRepository.cpp` | 271 | 257 | 257 |

## Method complexity interpretation

These are **physical line counts**, not McCabe cyclomatic complexity.
Comments are filtered approximately. To obtain accurate per-function
cyclomatic complexity, use a dedicated complexity analyser and record
its version, invocation and source SHA in a separate file.

**Review recommendation:** Inspect the longest repository/service
functions, especially submission transactions, moderation audit writes,
backup/restore staging and the role-specific Qt panels.

Do not report measured complexity figures without actually executing
an analysis tool.
