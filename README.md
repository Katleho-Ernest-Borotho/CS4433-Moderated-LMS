# CS4433 – Moderated Learning Management System

Individual Software Engineering project for a secondary-school Moderated Learning Management System (LMS).

## Project scope

- Four roles: Administrator, Lecturer, Head of Department (HoD), and Student.
- Management of users, departments, courses, enrolments and assignments.
- Assignment submissions as typed text, an attached file, or both.
- Individual extensions, resubmission history, late-work penalties, lecturer marking, written feedback, HoD moderation and controlled release of results.
- Audit history for changes to marks.

## Agreed Phase 1 deployment direction

- **Implementation:** C++17 desktop application with a simple GUI.
- **Persistence:** Local SQLite database accessed through a data-access layer.
- **Deployment:** Version 1.0 targets a single school computer and works without requiring Internet access for normal operation.
- **Deployment verification:** A second laptop will be used to check that the packaged program runs outside the development environment; it is not a second network client.
- **Build and quality tools:** CMake, Git, automated tests (GoogleTest or Catch2), static analysis and CI when implementation work begins.

## Project phase documents

| Phase | Folder | Status |
|---|---|---|
| 1 – Planning and management | `docs/phase-1-planning/` | Submitted PDF archived |
| 2 – Requirements engineering | `docs/phase-2-requirements/` | Submitted PDF archived |
| 3 – System design | `docs/phase-3-design/` | Planned |
| 4 – Implementation | `docs/phase-4-implementation/` | Planned after Gate 2 |
| 5 – Verification and validation | `docs/phase-5-verification-validation/` | Planned |
| 6 – Configuration management and QA | `docs/phase-6-configuration-qa/` | Planned |
| 7 – Deployment | `docs/phase-7-deployment/` | Planned; release v1.0 |
| 8 – Maintenance | `docs/phase-8-maintenance/` | Planned; change request CR1 and release v1.1 |

**Approval caution:** Submission of the Phase 2 report does not by itself prove Gate 1 was approved. Implementation must not begin until the design has passed Gate 2, as required by the assignment.

## Repository structure

```text
CS4433-Moderated-LMS/
├── docs/                 # Course deliverables by phase
├── src/                  # C++ source files (after Gate 2)
├── include/              # C++ headers (after Gate 2)
├── tests/                # Automated tests (during implementation/V&V)
├── scripts/              # Optional build/deployment utilities
├── .gitignore
├── GIT_WORKFLOW.md
└── README.md
```

## Getting started

See [GIT_WORKFLOW.md](GIT_WORKFLOW.md) for one-time repository setup and everyday commit/push instructions. This initial repository is a **documentation and version-control baseline**; no LMS functionality has been implemented yet.

## Privacy

The archived submitted PDFs contain student details. Keep the repository **private** unless you have deliberately reviewed the files and decided to publish them. Never commit real student assessment records, passwords, local database files, API tokens or other secrets.
