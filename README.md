# Moderated Learning Management System

An **offline Windows desktop application** for managing assignments, student submissions, marking and moderated results. Built in **C++17**, **Qt 6** and **SQLite**, with four permission-controlled user roles.

**[Download Windows app](https://github.com/Katleho-Ernest-Borotho/CS4433-Moderated-LMS/releases/download/v1.1/ModeratedLMS_v1.1_windows_ucrt64.zip)** · **[Watch demonstration](https://github.com/Katleho-Ernest-Borotho/CS4433-Moderated-LMS/releases/download/v1.1/Video.Project.1.mp4)** · **[Read project report](https://github.com/Katleho-Ernest-Borotho/CS4433-Moderated-LMS/releases/download/v1.1/Report_202322762.docx)** · **[All releases](https://github.com/Katleho-Ernest-Borotho/CS4433-Moderated-LMS/releases)**

## Features

| User | Features |
| --- | --- |
| **Administrator** | Manage users, departments, courses, lecturer assignments, student enrolments, backups and reporting |
| **Lecturer** | Create assignments, grant individual or class deadline extensions, review work, mark submissions and provide feedback |
| **Head of Department** | Moderate assessments, record reasons for mark adjustments, approve and release results |
| **Student** | View courses and deadlines, submit work, review submission versions, and see released results |

**Assessment flow:** Student submits → Lecturer records a provisional mark → HoD approves after the effective deadline → HoD explicitly releases → Student sees the result. Marks and feedback remain private until release.

The app supports text and PDF/DOCX/TXT attachments, versioned submissions, late-work penalties and mark-change audit records. Version **1.1** adds whole-class deadline extensions while respecting longer individual deadlines.

## Install on Windows

The Windows download is a **portable ZIP**, not an installer. You do not need a C++ compiler or MSYS2 to run it.

1. [Download the complete v1.1 ZIP](https://github.com/Katleho-Ernest-Borotho/CS4433-Moderated-LMS/releases/download/v1.1/ModeratedLMS_v1.1_windows_ucrt64.zip).
2. In File Explorer, **Extract All** to a normal folder.
3. Open the extracted application folder and run **`ModeratedLMS.exe`**. Keep `lms_schema.sql`, DLLs and the `platforms` folder together with the executable.
4. On first launch in a new Windows profile, create the initial Administrator with a password of at least **12 characters**.
5. Log in as Administrator, create user accounts, add a department and course, assign a Lecturer and HoD, then enrol Students.
6. Sign out and log in with a role-specific account to use the corresponding functions.

The database and attachments are saved under the **current Windows user's Local AppData**, separately from the program files. Replacing the extracted app does not automatically remove those records. Back up existing data before upgrading from v1.0; v1.1 applies a small additive schema migration on startup.

Full walkthrough: [User manual](docs/phase-7-deployment/ROLE_USER_MANUAL_V11.md) · [Backup and restore](OPERATIONS_BACKUP_REPORTS.md).

## Report, presentation and documentation

| Resource | Access |
| --- | --- |
| **Recorded presentation & live demo** (10 min 12 sec) | [Watch / download MP4](https://github.com/Katleho-Ernest-Borotho/CS4433-Moderated-LMS/releases/download/v1.1/Video.Project.1.mp4) |
| **Project report** (editable Word) | [Download Report_202322762.docx](https://github.com/Katleho-Ernest-Borotho/CS4433-Moderated-LMS/releases/download/v1.1/Report_202322762.docx) |
| **Application package** | [v1.1 Windows ZIP](https://github.com/Katleho-Ernest-Borotho/CS4433-Moderated-LMS/releases/download/v1.1/ModeratedLMS_v1.1_windows_ucrt64.zip) |
| **Four-role user guide** | [Installation, accounts and workflows](docs/phase-7-deployment/ROLE_USER_MANUAL_V11.md) |
| **Project documentation** | [Requirements, design, tests and deployment](docs/README.md) |
| **Editable architecture/UML diagrams** | [Diagram index](docs/report-diagrams/README.md) |

The demonstration uses fictional users and data.

## Architecture and technology

- **GUI:** Qt 6 Widgets
- **Core:** C++17, layered application services, domain rules, repository interfaces
- **Storage:** SQLite transactions, foreign-key relationships and local attachment files
- **Security:** libsodium password hashing, service-level permission checks and restricted result release
- **Build / test:** CMake, Ninja, MSYS2 UCRT64, GoogleTest, CTest and GitHub Actions
- **Quality checks:** cppcheck, gcovr and Lizard

```text
Qt UI → Application services → Domain rules / repository interfaces
                              → SQLite database + local attachments
```

Normal operation is **offline on one Windows computer**. Web/mobile access, automated grading and multi-computer concurrent use are outside the implemented scope.

## Source code

```text
assets/      Application icon
include/     C++ public headers and interfaces
src/         UI, application, domain, persistence, security and tools
schema/      Database schema and v1.1 migration
tests/       GoogleTest regression tests
scripts/     Packaging and metrics utilities
docs/        Planning, requirements, design, testing and deployment evidence
.github/     Continuous integration and Windows release automation
```

For a detailed build procedure, see [BUILD.md](BUILD.md). In **MSYS2 UCRT64** with the dependencies installed:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
./build/ModeratedLMS.exe
```

## Testing and releases

| Recorded check | Result |
| --- | --- |
| v1.0 baseline regression | **115/115 passed** |
| v1.1 class-extension regression | **128/128 passed** |
| v1.0 domain/application line coverage | **92%** (484/524 executable lines; not a measured v1.1 figure) |
| Login performance with 800 synthetic student records | **20/20 valid logins within 2 seconds** |
| Portable v1.1 package | Developer-reported successful testing on a second Windows laptop |

The release ZIPs are versioned under [GitHub Releases](https://github.com/Katleho-Ernest-Borotho/CS4433-Moderated-LMS/releases); the source tags `v1.0` and `v1.1` preserve their respective baselines. Further documentation and appearance changes in the development history do not alter those published packages.

**Status:** These are academic **release candidates**, not signed or formally approved production deployments. Existing limitations include single-computer operation, unencrypted backup folders, and outstanding formal accessibility and acceptance review. Use fictional data for testing and treat all actual student records as confidential.

---

Created and maintained as an individual software engineering project.
