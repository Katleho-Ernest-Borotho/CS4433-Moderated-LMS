# CS4433 — Final submission index

**Assignment requirement:** Submit **one PDF report of 6,000–8,000 words** (excluding tables, diagrams, appendices) and **the Git repository link**. Avoid replacing this with an unrequested ZIP of source code and evidence.

**Repository link:** https://github.com/Katleho-Ernest-Borotho/CS4433-Moderated-LMS

**Working report:** `CS4433_Final_Project_Report_202322762_DRAFT.pdf` has been generated in the conversation as a 21-page editable review deliverable (approximately 6,900 words before references and appendices). It is intentionally **not yet claimed as the signed final report**. Save the PDF in this folder if the university requests the report within the repository too; the GitHub connector cannot automatically fetch files from the developer's ChatGPT downloads.


## Accepted expanded draft and final evidence audit (2 November 2026)

The user has selected `CS4433_Final_Report_Expanded_Draft_202322762.docx` as the base document to retain for the final report. It is held as a conversation deliverable (not yet uploaded into this folder). Its prose must not be silently rewritten; remaining appendix work should use the following grounded records:

- [Final evidence and acceptance gap register](FINAL_EVIDENCE_AUDIT_2026-11-02.md)
- [Updated risk review, preserving the original 11 risk scores](UPDATED_RISK_REGISTER_2026-11-02.md)

**Release status correction:** GitHub now has **v1.0** and **v1.1 pre-releases**, each with a Windows ZIP and SHA-256 file. These are not signed/production-approved releases; formal acceptance remains pending.

## Final report sections

The master report follows the assignment's required order:
1. Title, abstract and table of contents
2. Introduction
3. Project management plan
4. Requirements engineering
5. Software design document
6. Implementation
7. Verification and validation
8. Configuration management and quality assurance
9. Deployment
10. Maintenance (CR1 / v1.1)
11. Professional, legal and ethical issues
12. Conclusion
13. References
14. Appendices, including traceability, test results, user manuals and risks

The original submitted Week 1–3 PDFs are versioned under `docs/phase-1-planning`, `docs/phase-2-requirements`, and `docs/phase-3-design`. The final report cites and summarises them rather than replacing the submitted baselines.

## Evidence inventory

| Requirement | Evidence location | Status |
|---|---|---|
| Must FR-01..FR-18 | `docs/phase-5-verification-validation/REQUIREMENTS_TRACEABILITY.md` | Mapped; tests passed |
| v1.0 115 GoogleTest cases | Phase 5 execution evidence | 115/115 PASS |
| 70% code coverage target | `docs/phase-5-verification-validation/COVERAGE_EVIDENCE_2026-11-02.md` | 92% application/domain measured |
| 800-account login requirement | `docs/phase-5-verification-validation/NFR01_PERFORMANCE_EVIDENCE_2026-11-02.md` | 20/20 within 2 seconds |
| cppcheck | `docs/phase-6-configuration-qa/STATIC_ANALYSIS_EVIDENCE_2026-11-02.md` | 29 files, no configured warnings |
| Code metrics / self-review | `docs/phase-6-configuration-qa/CODING_STANDARD_AND_SELF_REVIEW.md` and `scripts/phase6_metrics.py` | Run metrics tool and archive actual outputs |
| Deployment diagram | `docs/phase-7-deployment/DEPLOYMENT_DIAGRAM_V11.drawio` | Editable |
| Four-role user manual | `docs/phase-7-deployment/ROLE_USER_MANUAL_V11.md` | Drafted |
| CR1 v1.1 regression | `docs/phase-8-maintenance/CR1_TEST_EVIDENCE_2026-11-02.md` | 128/128 PASS |
| Independently packaged v1.1 build | `docs/phase-8-maintenance/CR1_V11_RELEASE_PACKAGING_EVIDENCE_2026-11-02.md` | SHA verified; user-reported second-laptop acceptance |
| Deployment/maintenance diagrams and release notes | Phase 7 / Phase 8 documents | Completed drafts |
| Supervisor approval and separately documented restore/migration | Handover checklist | Pending |

## Pre-submission quality gate

- Check the 6,000–8,000 body-word requirement after final edits.
- Replace any stale Draft/Review labels only **after** evidence gaps are resolved.
- Run `python scripts/phase6_metrics.py`, archive its exact output and commit SHA.
- Keep the full original V-Model system/acceptance test sheet, defect log and risk register.
- Capture screenshots from the actual v1.1 package (Lecturer, Student and independent Windows launch); avoid screenshots exposing passwords or pupil data.
- Document a safe offline Restore and an existing v1.0-profile migration using synthetic data; supervisor decides if these are required for Gate 3/final acceptance.
- Check third-party Qt/UCRT64 redistribution obligations and obtain supervisor sign-off.
- Do not merge, tag or publish a GitHub release without explicit approval.

## Local hand-in helper

Once the final PDF has been saved on your Windows machine, run:

```bash
python scripts/assemble_final_submission.py /c/Users/borot/Downloads/CS4433_Final_Project_Report_202322762_DRAFT.pdf
```

This helper makes a clean staging folder with **PDF + repository link + checksum**; it does **not** copy real school data or submission attachments. The assignment's official submission is the PDF and Git URL, not the staging ZIP.
