# Phase 8 — v1.1 Windows Release Packaging Evidence

**Observed date:** 2 November 2026  
**Build environment:** Windows MSYS2 UCRT64, `K4T7-Borotho`  
**Maintenance branch:** `feature/phase8-cr1-class-extensions`

## 1. Developer-supplied packaging output

The developer ran:

```bash
cd /c/Users/borot/OneDrive/Desktop/CS4433-Moderated-LMS
bash scripts/package_windows_ucrt64.sh
```

| Verification | Observed evidence | Status |
|---|---|---|
| CMake Release configuration | GNU C++ 16.2.0; SQLite 3.54.0; libsodium 1.0.22; Ninja generated | PASS |
| Compilation | `[37/37] Linking CXX executable ModeratedLMS.exe` | PASS |
| Qt deployment | `windeployqt6 --release` processed LMS and Restore executables | PASS |
| Qt Windows platform plugin | `platforms/qwindows.dll` included | PASS |
| External runtime libraries | Includes GCC runtime, `libsqlite3-0.dll`, `libsodium-26.dll`, and transitive DLLs | PASS |
| Output location | `ModeratedLMS_v1_1_20261102_023504` | CREATED |
| Independent Windows launch and requested CR1 checks | Developer subsequently reported all requested checks worked on second laptop | **USER-REPORTED PASS** |
| Verified backup on target | Developer subsequently reported the requested backup verification worked | **USER-REPORTED PASS** |

Generated folder:

```text
C:\dev\cs4433-lms-packages\ModeratedLMS_v1_1_20261102_023504
```

This package is produced from the v1.1 CR1 maintenance branch and is kept
separate from the previously tested v1.0 package.

## 2. Warnings

The Qt deployment utility printed that it skipped the OpenSSL backend and
could not locate `dxcompiler.dll` / `dxil.dll`, noting that the latter are
normally needed only for Direct3D 12 or related features. These warnings did
**not** prevent package creation. They do **not** prove portability either;
that requires independent launch and runtime testing.

## 3. Source update and CI

The developer separately supplied a successful `git pull --ff-only` on
`feature/phase8-cr1-class-extensions`, updating the local maintenance
branch to commit prefix `adbd8e1`.

GitHub Actions for commit
`adbd8e194fc0856e9e478880d385a1b1c14d0a5e` completed successfully:

https://github.com/Katleho-Ernest-Borotho/CS4433-Moderated-LMS/actions/runs/41378333994

**Build provenance confirmed** from the developer's subsequent `cat` output:

```text
Release: v1.1 CR1 / whole-class deadline extensions
Packaged (UTC): 2026-11-02T02:35:35Z
Git commit: adbd8e194fc0856e9e478880d385a1b1c14d0a5e
Build: Windows MSYS2 UCRT64 / CMake Release
Database migration: class_deadline_extensions added on first start
Notes: Not an installer; second laptop acceptance required
```

This commits the release package to an immutable, independently successful
Windows CI build revision. The later documentation-only commits do not
change the executable source included in this package.

## 4. Next acceptance procedure

1. Open `BUILD_PROVENANCE.txt` and record the actual Git commit.
2. Confirm that package includes `lms_schema.sql`,
   `migration-reference/001_class_deadline_extensions.sql`, two executables,
   and `platforms/qwindows.dll`. No live database or school work should
   be included.
3. Keep the complete v1.0 database/attachment backup and the original
   working v1.0 Release package available for rollback.
4. Transfer the **complete** v1.1 folder to the second laptop, without
   flattening plugin directories.
5. Perform V11-D01 through V11-D12 with demonstration records, record
   screenshots, and complete `CR1_RELEASE_BUILD_AND_ACCEPTANCE.md`.
6. Archive package provenance, test evidence and any defect/retest results.

## 5. Subsequent independent laptop verification

After transferring the packaged v1.1 folder to the second Windows laptop,
the developer replied **"and evrything worked"** following the requested
verification of: offline application launch, Lecturer whole-class extension,
two enrolled Students seeing the new Class due date, preservation of a
longer individual due date, application restart persistence, and
Administrator backup verification.

Record those requested steps as **developer-reported PASS**, not as
independently witnessed screenshot evidence. The developer supplied terminal
proof of successful packaging and source provenance; second-laptop UI
screenshots for this specific v1.1 package were not supplied in this turn.

The response does not separately document a **restore operation**, an
existing-v1.0-database migration on that second laptop, keyboard-only
accessibility, licence review, or supervisor approval. Those remain
individual release/handover checks rather than inferred passes.

**Conclusion:** v1.1 packaging and the requested independent deployment
scenarios have **passed as reported by the developer**. Formal handover,
remaining separately unverified checks, and supervisor acceptance are
still pending.
