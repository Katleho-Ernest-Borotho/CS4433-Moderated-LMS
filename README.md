# Moderated Learning Management System

CS4433 Software Engineering individual project.

This is a **C++17 desktop LMS** for a single Windows school computer. It
uses Qt 6 Widgets for the GUI, SQLite for offline data persistence, and
libsodium salted password hashes. There are four roles: Administrator,
Lecturer, Student and Head of Department.

## Implementation and phase work

The implementation branch is `feature/phase4-implementation`. It includes
authentication and user management, course/enrolment management, assignment
submission and version history, individual extensions, late penalties, lecturer
marking and feedback, HoD moderation/release, audit records, and administrator
backup/restore/reporting tools.

These features have been exercised on the development laptop. **This is not
yet a final validated v1.0 release.** Phase 5 system/acceptance measurements
and Gate 3 evidence remain required, as do Phase 6 QA, Phase 7 packaging and
Phase 8 whole-class extensions (CR1) for version 1.1.

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

The automated test suite currently contains 115 GoogleTest cases. This is
a source count: verify the actual CI/CTest results for the relevant commit.
No unmeasured coverage percentage or unperformed acceptance test is claimed.

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
