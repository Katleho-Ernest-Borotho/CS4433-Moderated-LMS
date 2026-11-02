# CS4433 — Final submission and acceptance evidence register

**Report baseline accepted for final editing:** `CS4433_Final_Report_Expanded_Draft_202322762.docx`. Its main prose is preserved; this is a supporting evidence register. **As of 2 November 2026**.

**Status meanings:** VERIFIED = dated tool evidence or linked file; USER-REPORTED = user confirmed work without individually archived logs/screenshots; PENDING = not separately executed/documented. Only observed results may be filled in.

| Assessed item | Evidence/source | Status | Remaining action |
|---|---|---|---|
| Week 1 feasibility, WBS, Gantt, 11 risks | Week 1 PDF | VERIFIED DOCUMENT | Standalone Gantt-image link still points to the PDF |
| FR-01–FR-18, NFR-01–NFR-06, use cases | Week 2 SRS and Phase 5 traceability | VERIFIED DOCUMENT | Audit all use cases against full matrix in final report |
| Design, UML, database | Week 3 SDD and original draw.io/PNG assets | VERIFIED DOCUMENT | Figure/photo source links already published |
| v1.0 integration tests | Phase 5 execution evidence | VERIFIED — 115/115 | Include dated console/CI XML result as full results appendix |
| v1.0 domain/application coverage | gcovr evidence | VERIFIED — 484/524 lines, 92% | Do not label this v1.1 coverage |
| NFR-01 benchmark | Phase 5 benchmark | VERIFIED — 20/20 under 2 s, 800 synthetic accounts | Record hardware specification if required |
| v1.1 CR1 regression | Phase 8 evidence and CI | VERIFIED — 128/128 | Include CI link and count in test appendix |
| Static analysis and complexity | Phase 6 documents | VERIFIED — cppcheck 29 sources; 311 functions avg CCN 3.6 | Reviewer sign/date still required |
| v1.0 and v1.1 tagged Windows ZIPs | GitHub Releases | VERIFIED pre-releases | Keep signed/approved status distinct from pre-release |
| Second-laptop v1.1 launch, CR1 two-student due dates, restart, backup verification | Phase 8 handover summary | USER-REPORTED PASS | Archive screenshots and device/OS information |
| ST-01–ST-16 complete manual system cases | System acceptance / defect log | PENDING comprehensive record | Record each executed test, failure/defect, screenshots |
| NFR-02 full offline test for every role | V-Model test plan | USER-REPORTED partial | Repeat formal four-role disconnected test and record |
| NFR-03 hash security database inspection | Tests + NFR plan | AUTOMATED VERIFIED; manual inspection PENDING | Inspect only synthetic database for hashed values |
| NFR-04 usability: four independent role users | SRS + test plan | PENDING | Four testers; ≤15 min induction; one unaided role task each |
| NFR-05 keyboard accessibility | SRS + test plan | PENDING | Tab, Shift+Tab, Enter and Space across primary controls |
| NFR-06 exact 20-record restart comparison | SRS + test plan | PENDING | Compare 20 numbered synthetic submissions/marks/audits before vs after restart |
| V11-D07 equal or earlier extension rejected | CR1 acceptance checklist | PENDING manual | Test against synthetic assigned course; no record change |
| V11-D08 new class extension after marking blocked | CR1 acceptance checklist | PENDING manual | Verify existing mark/deadline retained |
| V11-D11 verified offline restore | CR1 acceptance checklist | PENDING manual | Use dedicated test Windows profile and disposable demo records |
| V11-D12 actual v1.0 to v1.1 profile upgrade | CR1 acceptance checklist | PENDING manual | Backup v1.0 profile first; preserve original twelve tables/data |
| Restore lock while app running | Phase 7 D-09 | PENDING manual | Restore utility must refuse while LMS open |
| Redistribution licence notices | Phase 7 handover | PENDING | Verify Qt/MSYS2/libsodium runtime obligations; no production claims |
| Supervisor Gate 1/2/3 and release acceptance | Original phase gates | PENDING | Obtain real review/decision dates and signatures |
| Final PDF, exact body word count, links, submitted URL | Assignment §7 | IN PROGRESS | Export accepted draft plus appendix enhancements and verify links |

## Test execution record for real observations (do not pre-fill PASS)

- Device / Windows edition / architecture:
- Tester and date:
- ZIP release tag and SHA-256:
- Source SHA from release provenance:
- Synthetic dataset location and backup:
- Wi-Fi/Ethernet disconnected for offline check?:
- Test case ID:
- Steps performed:
- Actual outcome:
- PASS / FAIL / BLOCKED:
- Screenshot filename (exclude credentials and personal data):
- Defect ID, if needed:
- Retest result (if repaired):
- Supervisor signature/status: 

### Safe test order
1. Preserve both release ZIPs, original data and an independently verified backup.
2. Create a **separate test Windows user profile** with only fictional records.
3. Finish V11-D07 / D08 (negative CR1 GUI cases), then NFR-04 / NFR-05.
4. Run the NFR-06 20-record restart comparison.
5. Separately perform the restore-lock and offline-restore scenarios on disposable demo data.
6. Perform a v1.0→v1.1 migration on a different disposable profile created using v1.0, **never on the real user's live database**.
7. Record results, defect severities and any supervisor decisions.

Source links: [Phase 5 manual test record](../phase-5-verification-validation/SYSTEM_ACCEPTANCE_AND_DEFECT_LOG.md), [CR1 second-PC checklist](../phase-8-maintenance/CR1_RELEASE_BUILD_AND_ACCEPTANCE.md), [final Week 1 risk review](UPDATED_RISK_REGISTER_2026-11-02.md), [GitHub v1.0/v1.1 releases](https://github.com/Katleho-Ernest-Borotho/CS4433-Moderated-LMS/releases).
