# Phase 7 — Windows Release Packaging Evidence

**Observed date:** 2 November 2026 (timestamp in developer-provided package path)
**Development machine:** K4T7-Borotho, Windows, MSYS2 UCRT64
**Source location:** `C:/Users/borot/OneDrive/Desktop/CS4433-Moderated-LMS`

## Packaging execution

The developer executed:

```bash
cd /c/Users/borot/OneDrive/Desktop/CS4433-Moderated-LMS
bash scripts/package_windows_ucrt64.sh
```

The supplied terminal log shows:

- CMake configured a **Release** build using GNU C++ **16.2.0**.
- Detected SQLite **3.54.0**, pkg-config **3.0.10**, libsodium **1.0.22**.
- The build completed at **[37/37] Linking CXX executable ModeratedLMS.exe**.
- `windeployqt6` processed both `ModeratedLMS.exe` and `ModeratedLMSRestore.exe`.
- Qt libraries/plugins deployed: Qt6Core, Qt6Gui, Qt6Widgets, Qt6Network, Qt6Svg, the Windows platform plugin `platforms/qwindows.dll`, image formats and other plugins.
- Dependency collection copied GCC/UCRT64 and third-party DLLs, including `libstdc++-6.dll`, `libgcc_s_seh-1.dll`, `libwinpthread-1.dll`, `libsodium-26.dll`, `libsqlite3-0.dll` and their transitive dependencies.
- Last script output: **Prepared deployment folder**.

## Generated artifact

```text
C:\dev\cs4433-lms-packages\ModeratedLMS_v1_0_20261102_013723
```

The script is designed to include the LMS and Restore executables, `lms_schema.sql`, Qt plugins/translations and non-Qt DLLs. It excludes the development SQLite database, submission attachments and the optional Administrator password recovery executable.

### Warnings observed

`windeployqt6` did not include `qopensslbackend.dll` automatically. It also warned that `dxcompiler.dll` and `dxil.dll` were not found, noting this is not an issue unless Direct3D 12 or certain graphics features are used.

For this offline Qt Widgets desktop application, these messages do not by themselves establish a failure; successful independent launch is still required.

## Evidence status

| Check | Result |
|---|---|
| Release configuration | PASS — CMake generated build |
| Build | PASS — completed 37/37 |
| Qt deployment | PASS — processed two executables |
| UCRT64 DLL collection | PASS — script completed |
| Generated directory | PASS — path printed |
| Second laptop GUI launch | **OBSERVED PASS** — Initial Administrator setup window visibly open in supplied photograph; toolchain independence still to verify |
| First-run setup screen | **OBSERVED PASS** — Administrator setup form displayed |
| Administrator setup, academic workflow, persistence, offline operation | **USER-REPORTED PASS** on second laptop (four acceptance checks) |
| Windows-specific redistributable license review | **NOT COMPLETED** |

This log validates package construction **on the development laptop**, not portability. Do not call v1.0 fully deployed until the second Windows laptop acceptance checklist is completed and signed off. A source commit SHA was not provided in the terminal log; capture `git rev-parse HEAD` for final artifact provenance.

## Photograph-based second laptop launch evidence

The developer supplied a photograph of the second Windows laptop after extracting
`ModeratedLMS_v1_0_20261102_013723`. The photograph visibly shows:

- The **Moderated Learning Management System** application open.
- The **Initial administrator setup** form, including username and password fields.
- File Explorer in the extracted deployment directory with `lms_schema.sql`,
  two application executables, and several packaged DLLs.

**Observed result:** The Windows GUI launches and reaches first-run setup.
This supports D-01 (GUI-launch portion), D-02 (schema present), and D-03
(initial-setup screen). It does **not** independently establish that MSYS2 or
other developer runtimes are absent, that the computer is offline, that a first
Administrator can be created, or that records persist after restart.

The photograph is held in the project conversation; it has not been
committed to the repository because the image also shows unrelated desktop
information. If needed, save a tightly cropped evidence copy separately.

**Next:** Complete setup using synthetic credentials, verify all four roles,
restart persistence and offline operation, and then record the outcomes in
`WINDOWS_RELEASE_AND_SECOND_LAPTOP_TEST.md`.

## Follow-up execution report from second laptop

The developer subsequently reported that the following acceptance checks
**all passed** on the independent Windows laptop:

1. Initial Administrator account setup and login.
2. Test academic workflow (users, roles, course/enrolment/assignment operations).
3. Records retained after fully closing and reopening the application.
4. Core functionality with the laptop offline.

**Evidence provenance:** these four outcomes are recorded from the user's
explicit confirmation; only the first-run application launch was accompanied
by a photograph. No per-test screenshots, timings or independent installation
inventory have been supplied for the four subsequent results. Record them as
**tester-reported PASS**, not externally audited results.

Full D-01–D-12 checklist, restore lock test, full four-role end-to-end
release and redistribution/licensing review remain separate acceptance
items. The v1.0 branch remains untagged while those items are pending.
