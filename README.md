# Moderated Learning Management System

CS4433 Software Engineering individual project.

This is a **C++17 desktop LMS** for a single Windows school computer. It
uses Qt 6 Widgets for the GUI, SQLite for offline data persistence, and
libsodium salted password hashes. There are four roles: Administrator,
Lecturer, Student and Head of Department.

## Current branch: v1.1 (CR1 whole-class extensions)

This maintenance branch (`feature/phase8-cr1-class-extensions`) extends
the original v1.0 codebase with **whole-class assignment deadline extensions**.
The existing individual-extension rule, 12 original database tables,
role permissions and marking/release workflow remain intact. Startup
adds one new SQLite table for v1.1.

**Verified:** 128/128 local GoogleTest/CTest cases, Windows UCRT64 CI
for the implementation checkpoint, and developer-reported GUI acceptance.
The v1.0 package previously launched and worked on a second Windows
laptop. **The separate v1.1 Release package and second-laptop test are
not complete yet.** Do not confuse these two deployment results.

For the maintenance-specific design, editable diagrams, evidence, and
Windows release preparation, see
[Phase 8 CR1 documents](docs/phase-8-maintenance/).

The v1.0 implementation baseline is retained separately in
`feature/phase4-implementation`. Do not overwrite its signed-off
design documents or delete the verified rollback backup.

## Build and verification

- Local build: see [BUILD.md](BUILD.md).
- Phase 5 V-Model plan: [TEST_PLAN.md](docs/phase-5-verification-validation/TEST_PLAN.md).
- FR-01..FR-18 and NFR-01..NFR-06 mapping:
  [REQUIREMENTS_TRACEABILITY.md](docs/phase-5-verification-validation/REQUIREMENTS_TRACEABILITY.md).
- Manual acceptance execution and defect log:
  [SYSTEM_ACCEPTANCE_AND_DEFECT_LOG.md](docs/phase-5-verification-validation/SYSTEM_ACCEPTANCE_AND_DEFECT_LOG.md).
- Automated build/test/coverage/static-analysis workflow:
  [.github/workflows/ci.yml](.github/workflows/ci.yml).
- Backup/restore operation: [OPERATIONS_BACKUP_REPORTS.md](OPERATIONS_BACKUP_REPORTS.md).
- Administrator password recovery: [ADMIN_RECOVERY.md](ADMIN_RECOVERY.md).

The v1.1 suite contains **128 GoogleTest cases**, all of which passed
locally in **53.26 seconds** at the recorded checkpoint. For a new revision,
rerun CTest and check the exact CI SHA. Version 1.1 GUI acceptance was
reported by the developer, while independent v1.1 deployment acceptance
remains outstanding. Do not reuse the v1.0 92% coverage percentage as a
measured v1.1 value without an updated gcovr run.

## Phase documents

- `docs/phase-1-planning/` — planning and risk management.
- `docs/phase-2-requirements/` — submitted SRS and use-case model.
- `docs/phase-3-design/` — software design and database schema.
- `docs/phase-4-implementation/` — implementation evidence.
- `docs/phase-5-verification-validation/` — test strategy and evidence.
- `docs/phase-6-configuration-qa/` — configuration and quality assurance.
- `docs/phase-7-deployment/` — installation and release documents.
- `docs/phase-8-maintenance/` — CR1 change impact and v1.1 regression.

Do not put real learners' data or password-bearing backup files into this Git
repository. The supplied tests should use fictional demonstration records.
