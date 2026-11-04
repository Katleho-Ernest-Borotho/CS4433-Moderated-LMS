# Phase 5 — NFR-01 Performance Evidence (2026-11-02)

The isolated benchmark ran on the development Windows MSYS2 UCRT64 machine with an SQLite fixture containing **800 Student accounts**.

| Measure | Observed |
|---|---:|
| Required within two seconds | 19/20 valid logins |
| Actual within two seconds | **20/20** |
| Minimum | 48.36 ms |
| Maximum | 53.30 ms |
| Mean | 50.32 ms |
| NFR-01 result | **PASS** |

Twenty observed times (ms): 48.74, 51.52, 50.07, 49.48, 48.96, 48.94, 49.40, 48.36, 50.11, 49.45, 49.16, 51.25, 49.40, 50.18, 51.91, 50.37, 52.17, 53.30, 52.27, 51.45.

This measures valid authentication only, not the full GUI dashboard transition. The benchmark uses a temporary database rather than real Student records.

Capture `git rev-parse HEAD` and retain terminal output/screenshot for exact commit provenance.