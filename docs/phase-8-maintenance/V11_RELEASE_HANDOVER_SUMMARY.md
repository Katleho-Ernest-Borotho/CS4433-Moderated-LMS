# CS4433 Moderated LMS — v1.1 Release Candidate Handover Summary

**Date of evidence:** 2 November 2026  
**Status:** Technically implemented and independently user-tested; **formal sign-off pending**  
**Scope:** Original v1.0 LMS plus CR1 whole-class deadline extension (v1.1)  
**Windows target:** Offline, single-computer Qt6/C++17 desktop application.

## Release identity

| Field | Recorded value |
|---|---|
| Package name | `ModeratedLMS_v1_1_20261102_023504` |
| Build timestamp (UTC) | `2026-11-02T02:35:35Z` |
| Verified packaged Git commit | `adbd8e194fc0856e9e478880d385a1b1c14d0a5e` |
| Branch | `feature/phase8-cr1-class-extensions` |
| Compiler | GNU C++ 16.2.0 / MSYS2 UCRT64 |
| Database | SQLite 3.54.0; original 12 tables plus one CR1 table |
| Qt deployment | `windeployqt6` DLLs/plugins included |
| License/signing | Not yet approved/signed for general distribution |

**Provenance:** These values were read from the actual developer-provided
`BUILD_PROVENANCE.txt` in the generated package, not inferred from the
current branch head.

## Evidence dashboard

| Gate | Actual evidence | Status |
|---|---|---|
| v1.0 local automated tests | 115/115 passed | PASS |
| v1.0 measured business-logic line coverage | 484/524 lines, 92% | PASS (70% requirement) |
| NFR-01 performance | 20/20 logins below 2s, 800 Student records | PASS |
| Phase 6 cppcheck | 29/29 source files, no findings in selected categories | PASS within selected scan configuration |
| v1.0 second Windows PC | Developer confirmed setup, academic workflow, persistence, offline operation | DEVELOPER-REPORTED PASS |
| CR1 implementation regression | 128/128 tests passed, 53.26s | PASS |
| v1.1 Windows CI for packaged source commit | Successful GitHub Actions run | PASS |
| v1.1 Release build / DLL packaging | Successful `[37/37]` Release build, Qt/plugins and runtime DLLs copied | PASS |
| v1.1 packaged commit provenance | Verified from `BUILD_PROVENANCE.txt` | PASS |
| v1.1 second Windows PC | Developer reported all *requested* scenarios worked: offline launch, class extension, two Students' class due dates, longer individual due date, restart persistence, backup verification | DEVELOPER-REPORTED PASS |
| Negative GUI cases, offline restore, existing v1.0 profile migration on second PC | Not individually documented with screenshots/results in this checkpoint | NEEDS SEPARATE EVIDENCE |
| Final supervisor acceptance and redistribution licences | No approval recorded | PENDING |

The successful v1.0 coverage percentage must not be silently reused as
measured **v1.1** coverage. The v1.1 build/CI and regression suite passed;
extract an updated gcovr report if a v1.1 coverage figure is required.

## Version 1.1 user-visible changes

- Lecturer can extend the deadline of an assignment for the whole class.
- Student assignment views show the Class due and Your due dates separately.
- A longer individual extension is never shortened by a class extension.
- The effective deadline is the latest of the original, class and individual deadlines.
- Submission, resubmission, penalties and HoD release use that deadline.
- The application adds `class_deadline_extensions` while preserving the
  original twelve tables.
- Updates after marking begins are rejected to avoid altering recorded penalties.

## Handover file map

- [Release packaging evidence](CR1_V11_RELEASE_PACKAGING_EVIDENCE_2026-11-02.md)
- [Release notes](CR1_V11_RELEASE_NOTES.md)
- [Design delta / ERD](CR1_V11_DESIGN_DELTA.md)
- [Change impact and regression](CR1_CHANGE_IMPACT_AND_REGRESSION.md)
- [Test evidence](CR1_TEST_EVIDENCE_2026-11-02.md)
- [Independent Windows acceptance checklist](CR1_RELEASE_BUILD_AND_ACCEPTANCE.md)
- [Editable v1.1 ERD](diagrams/CR1_ERD_v1_1.drawio)
- [Editable v1.1 service/class diagram](diagrams/CR1_SERVICE_CLASS_DELTA_v1_1.drawio)
- [Windows package creation script](../../scripts/package_windows_ucrt64.sh)

## Required pre-submission tasks

1. Retain **both** original v1.0 and confirmed v1.1 Release folders.
   Keep the verified full database/attachment backup for rollback.
2. Save clean, dated screenshots of Lecturer class extension and both
   Students' due dates. Include an image of the second-PC app running.
3. Record any negative acceptance results still required by the rubric:
   equal/earlier extension rejection, marked-assignment rejection,
   restore-lock rejection, and HoD deadline enforcement.
4. On synthetic data, test an **offline v1.1 restore** and separately
   demonstrate the migration of a previously created v1.0 database.
   These are distinct from the developer's reported backup verification.
5. Review third-party licence notices and the final release package's
   redistribution obligations. There is no signed Windows installer.
6. Obtain the supervisor's release/acceptance decision. Record the
   decision and defects, if any, before creating a final release tag.

## Formal acceptance record

| Field | Value to complete |
|---|---|
| Submitted package checksum (SHA-256) | |
| Submitted source commit | `adbd8e194fc0856e9e478880d385a1b1c14d0a5e` |
| Second Windows laptop and OS edition | |
| Complete V11-D01..D12 evidence archive | |
| Outstanding Critical or High defects | |
| Supervisor name / date / signature | |
| Release decision | PENDING |

**Release decision:** The main implementation and requested cross-device
deployment checks are complete at the developer-reported evidence level. This
record **does not** claim a completed formal sign-off or a published final
release. No tag or public release has been created automatically.
