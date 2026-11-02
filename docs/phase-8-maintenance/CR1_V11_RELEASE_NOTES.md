# Moderated Learning Management System — v1.1 Release Candidate Notes

**Status:** v1.1 independently tested release candidate; formal supervisor sign-off outstanding  
**Branch:** `feature/phase8-cr1-class-extensions`  
**Change request:** CR1, whole-class assignment deadline extensions  
**Baseline:** v1.0 offline single-PC Windows desktop LMS  
**Release evidence:** `docs/phase-8-maintenance/CR1_TEST_EVIDENCE_2026-11-02.md`

## What's new

**Whole-class deadlines.** A Lecturer assigned to a course can extend
an assignment deadline for every enrolled Student in one action through
**Assignments and extensions → Whole-class deadline extension**.
The Lecturer must choose a new later date and confirm the action.

**Preserve individual accommodations.** A Student's personal due date is
the latest of the original assignment deadline, the class extension and the
Student's own individual extension. Extending the whole class never shortens
a longer approved individual deadline.

**Clear Student deadline visibility.** Students see the class-wide date
in the new **Class due (local)** column and their own effective deadline
in **Your due (local)**.

**Consistent assessment and release.** The effective deadline applies to
first submissions, versioned resubmissions, lateness deductions, and HoD
moderation. Further class extensions are blocked after any assessment
result has been created for that assignment to avoid retroactively
changing penalties or released marks.

**Additive schema migration.** The `class_deadline_extensions` table is
added transactionally on startup. The 12 v1.0 tables remain intact;
v1.1 uses **13 application tables**.

## Compatibility and precautions

- Keep the original working v1.0 Release package.
- Make and verify a full v1.0 backup (SQLite + submitted files) before
  launching v1.1 under a Windows account with existing LMS data.
- Version 1.1 upgrades its existing database on first launch. Do not
  attempt to run the old executable against an upgraded database as a
  substitute for a documented rollback.
- To return to v1.0: close the LMS, restore the verified v1.0 snapshot
  with the offline Restore utility, then use the original v1.0 executable.
- Windows Release packages contain dependencies but **not** actual school
  data, private attachments, test account passwords or the optional
  Administrator recovery executable.
- Third-party licence notices must be reviewed before distribution.
- There is no network service, automated notification, or installer in
  this release candidate.

## Verification status

| Evidence | Status |
|---|---|
| 128-case local regression suite | **PASS** |
| Windows UCRT64 CI at implementation checkpoint | **PASS** |
| Developer's manual CR1 GUI confirmation | **REPORTED PASS** |
| Upgrading stored v1.0 data without account loss | **Automated test PASS** |
| v1.1 Release package generated | **PASS** — Windows Release deployment package created |
| v1.1 on second Windows laptop | **USER-REPORTED PASS** — offline launch, CR1 dates, persistence, backup verification |
| Supervisor sign-off/release tag | **PENDING** |

The v1.0 second-laptop deployment was tested and reported working; that
evidence is not automatically transferable to the new v1.1 binary.

## Documentation

- `CR1_CHANGE_IMPACT_AND_REGRESSION.md` — change request and impact
- `CR1_V11_DESIGN_DELTA.md` — ERD, services and behavioural design
- `diagrams/CR1_ERD_v1_1.drawio` — editable ERD update
- `diagrams/CR1_SERVICE_CLASS_DELTA_v1_1.drawio` — editable class/service update
- `CR1_TEST_EVIDENCE_2026-11-02.md` — observed test results
- `CR1_RELEASE_BUILD_AND_ACCEPTANCE.md` — packaging and final testing instructions

## Gate for final v1.1 release

**Package provenance and independent test recorded:** build commit
`adbd8e194fc0856e9e478880d385a1b1c14d0a5e`, packed
`2026-11-02T02:35:35Z`, and user-reported second-laptop success.

**Before final release:** archive the second-laptop screenshots/test log,
separately execute and document the offline restore and real v1.0→v1.1
migration acceptance on an expendable test profile, review redistribution
licences, and obtain supervisor approval. The repository's maintenance
branch is still a release candidate, not a tagged final handover.
