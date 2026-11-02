# Phase 6 — Coding Standard, Metrics and Self Code Review

## Coding standard and configuration controls

- C++17, four-space indentation and braces on structured function/class blocks; coherent interface names `I...`, PascalCase class types, lowerCamelCase operations.
- Keep Qt Widgets focused on presentation and input. Authentication, late penalties and result-release decisions belong in application/domain classes. All database queries stay in repository/data-access classes.
- Use RAII (`unique_ptr`, scoped SQLite statements and transactions) where possible. Bound SQL parameters and rollback on failed writes protect consistency.
- Do not log plaintext passwords, password reset inputs or private student submission bodies. Salted libsodium hashes are stored, not reversible passwords.
- Validate roles and actual course/dept assignments inside services; hiding a button is not authorisation.
- Use UTF-8, UTC database timestamps, local GUI formatting, meaningful Git commits and automated GoogleTest/CTest validation.

## Reproducible code metrics

Run `python scripts/phase6_metrics.py` from the repository root.
It writes `docs/phase-6-configuration-qa/MEASURED_CODE_METRICS.md`
with per-layer source file counts, physical lines, nonblank noncomment
lines, longest source files, and key-method physical spans. It does **not**
pretend physical span is cyclomatic complexity; the script also saves
`cppcheck` command guidance for further complexity evaluation.

Do not fill a measured result until the command has actually been run on
the chosen source commit; include the commit SHA and run timestamp.

## Self code-review checklist

| Area | Concrete inspection | Evidence already present / action |
|---|---|---|
| User model and polymorphism | Inspect abstract `User` and four derived role types | Designed in Week 3; source review remains part of viva preparation |
| Authorisation | Service `requireRole`, course/dept lookup on every protected operation | Auth/role and end-to-end regression tests passed |
| Database and queries | Parameterised statements, atomic result + audit changes, additive CR1 migration | 128/128 tests including v1.0 migration and audit rollback |
| Authentication | Salted libsodium hash, disabled-user block, session invalidation | Password and account-recovery regression tests passed |
| Data-loss controls | Full backup with SQLite + attachments, integrity manifest and offline restoration | Packager and backup tests pass; separate target restore acceptance still to be documented |
| Assessment policy | Boundary handling at 0,24,48,72h; penalty from maximum mark, max 72h | Penalty policy tests pass; compare with SRS BR-02 |
| Marks release | HoD approval before separate release; no Student access to provisional marks | Permission and moderation tests pass |
| CR1 | Class/individual max-deadline rule; reject extensions after marking | 13 new CR1 automated tests passed |
| Error handling | Controlled input exceptions and GUI error dialogs | End-to-end tests; manual exploratory testing still recommended |
| Code metrics | Actual LOC and complexity figures with tool output | **Pending until metrics script is run** |
| CI/static analysis | UCRT64 GitHub Actions, cppcheck, gcovr | 128/128 regression; v1.0 92% measured coverage; 29 cppcheck files no findings in chosen categories |
| Configuration provenance | Exact packaged Git SHA, build timestamp, rollback backup | v1.1 package SHA `adbd8e1...` verified |

## Review changes and limitations

Changes already implemented in earlier checkpoints include renaming/reactivation
and password-reset service permissions, invalidating old sessions when the
password hash changes, adding an offline Restore utility under an application
lock, and preventing whole-class extension after an assessment result exists.

A clean cppcheck scan in the configured warning/performance/portability
categories is evidence of tool completion; it cannot establish absence of
all security defects. File-content antivirus scanning, encrypted backups,
automated retention policy, code signing, additional accessibility assessment
and formal supervisor sign-off are not claimed as completed. Before release,
review redistribution obligations for Qt, SQLite, libsodium and UCRT64 DLLs.

## Recommended commands

```bash
# MSYS2 UCRT64 or Git Bash with Python available
python scripts/phase6_metrics.py

# MSYS2 UCRT64 (show non-zero errors explicitly)
cppcheck --enable=warning,performance,portability --std=c++17 \
  --suppress=missingIncludeSystem -I include --error-exitcode=1 src
```

Record code-review date, reviewer, SHA, findings, changes, and re-test result
in a signed report before supervisor handover; automated source metrics are
not a substitute for reviewing the logic of sensitive operations.
