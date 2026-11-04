# CS4433 — Final risk review (2 November 2026)

**Basis:** submitted Week 1 project management plan, Section 5 (11 original risks, owner: Student developer). The **initial likelihood, impact and exposure scores are preserved** below. This is an evidence-based final review, not a fabricated Gate 1/2/3 sign-off. New *residual numerical* ratings have not been collected; therefore none is assigned.

Scales: likelihood L = 1–5; impact I = 1–5; **initial exposure = L × I**.

| Risk | Original category and threat | L | I | Exposure | Owner | Actual controls/evidence available | Current status / unresolved item |
|---|---|---:|---:|---:|---|---|---|
| R1 | Project: seven-week schedule slips | 4 | 5 | 20 | Student developer | Phased baselines, CMake/CI and published v1.0/v1.1 pre-releases | Final PDF hand-in and viva remain |
| R2 | Project: conflicting stakeholder requirements cause rework | 4 | 4 | 16 | Student developer | FR-01–FR-18 SRS, three use cases, dated traceability matrix | Formal Gate 1 approval not documented |
| R3 | Technical: GUI/C++/CMake/SQLite integration delays work | 3 | 4 | 12 | Student developer | Windows UCRT64 CMake build and CI passed, packaged Qt and dependency closure | Recheck final package on target machine if changed |
| R4 | Technical: password/assessment records exposed | 3 | 5 | 15 | Student developer | libsodium salted hashes, service-layer role checks, SQLite bound queries, audit tests | Real operational access policies and data-handling acceptance pending |
| R5 | Technical: poor performance on old computers | 3 | 4 | 12 | Student developer | 20/20 logins below 2s with 800 synthetic Students on development PC | Age/condition of actual target hardware unverified |
| R6 | Technical/project: poor Internet interrupts Git/CI | 3 | 3 | 9 | Student developer | Local Git workflow and successful Windows CI; LMS works offline as designed | Remote repository access still needed for hand-in |
| R7 | People: illness/academic load reduces time | 2 | 5 | 10 | Student developer | Prioritised phased submissions, extensive test automation | Schedule risk persists until submission |
| R8 | Project: loss/corruption of source, report or database | 2 | 5 | 10 | Student developer | GitHub source, release tags/assets, backup verification tests, manifest integrity | Independent offline restore still to be documented |
| R9 | Project: Gate 2 delayed or design not accepted | 2 | 5 | 10 | Student developer | Week 3 detailed SDD and editable diagrams | Explicit supervisor Gate 2 approval not present in records |
| R10 | Project: extra features create scope creep | 2 | 4 | 8 | Student developer | Single-PC offline scope maintained; CR1 limited to lecturer class extensions | Do not add functionality before submission |
| R11 | Technical/deployment: package fails on independent PC | 2 | 3 | 6 | Student developer | v1.0/v1.1 Windows ZIPs; second-laptop launch and selected scenarios developer-reported PASS | Negative GUI cases, upgrade and restore still unrecorded |

## Review conclusion
Eleven risks are retained without altering their original scores. Controls are documented, but **mitigation is not equal to formal closure**. If a supervisor signs a Gate review, record its date, residual ratings, sign-off and any newly found defects separately. Do not invent historical ratings.

Source: [Week 1 submitted project management plan](../phase-1-planning/Week1_Submitted_Project_Management_Plan.pdf).

