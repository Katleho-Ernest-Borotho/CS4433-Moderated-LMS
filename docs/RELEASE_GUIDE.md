# Windows releases

Prebuilt Windows packages are available under [GitHub Releases](https://github.com/Katleho-Ernest-Borotho/CS4433-Moderated-LMS/releases). These are **portable release candidates**, not signed installers.

## Published versions

| Version | Source revision | Automated tests | Contents |
| --- | --- | --- | --- |
| [v1.1](https://github.com/Katleho-Ernest-Borotho/CS4433-Moderated-LMS/releases/tag/v1.1) | `adbd8e194fc0856e9e478880d385a1b1c14d0a5e` | 128/128 passed | Four-role LMS, whole-class deadline extensions, report and demonstration |
| [v1.0](https://github.com/Katleho-Ernest-Borotho/CS4433-Moderated-LMS/releases/tag/v1.0) | `0f5ae44003fbea1583595a1afc8b59a376b2e64c` | 115/115 passed | Original moderated assignment and result-release workflows |

### v1.1 downloads

- **[Portable Windows application ZIP](https://github.com/Katleho-Ernest-Borotho/CS4433-Moderated-LMS/releases/download/v1.1/ModeratedLMS_v1.1_windows_ucrt64.zip)** — extract the whole folder and run `ModeratedLMS.exe`.
- **[Project report (Word)](https://github.com/Katleho-Ernest-Borotho/CS4433-Moderated-LMS/releases/download/v1.1/Report_202322762.docx)** — design, implementation and verification summary.
- **[Recorded presentation and walkthrough (MP4)](https://github.com/Katleho-Ernest-Borotho/CS4433-Moderated-LMS/releases/download/v1.1/Video.Project.1.mp4)** — a 10-minute 12-second demonstration.
- **[SHA-256 checksums](https://github.com/Katleho-Ernest-Borotho/CS4433-Moderated-LMS/releases/download/v1.1/SHA256SUMS.txt)** — checksum for the application ZIP.

These files are **release assets**, not GitHub Packages. Because both versions are marked **pre-release**, GitHub may show only `2 tags` in the repository sidebar. The [Releases page](https://github.com/Katleho-Ernest-Borotho/CS4433-Moderated-LMS/releases) lists their full download assets.

For installation and usage, see the [main README](../README.md) and [role-based user manual](phase-7-deployment/ROLE_USER_MANUAL_V11.md).

## Reproducible build and packaging

The release workflow is [`.github/workflows/publish-windows-release.yml`](../.github/workflows/publish-windows-release.yml). It selects an exact source commit, verifies the expected automated tests on Windows, builds the application using MSYS2 UCRT64 and CMake, bundles Qt/runtime dependencies, records provenance, creates a ZIP and calculates its SHA-256 checksum.

The v1.0 and v1.1 release source snapshots are intentionally separate from the evolving `main` branch. Published packages are CI builds of those pinned commits, not exports of later source revisions.

### Publishing a future version

A new version requires its own tested source revision, tag, and release entry. The existing workflow is configured specifically for the published v1.0/v1.1 baselines and refuses to overwrite an already published release.

The application does not bundle real students' databases, attachments or passwords. Backups and Windows account access require appropriate protection. Formal supervisory approval, some manual acceptance evidence and third-party licence review remain outstanding; do not present a release candidate as a production-approved deployment.

[Browse releases](https://github.com/Katleho-Ernest-Borotho/CS4433-Moderated-LMS/releases) · [GitHub Actions](https://github.com/Katleho-Ernest-Borotho/CS4433-Moderated-LMS/actions) · [Build instructions](../BUILD.md)
