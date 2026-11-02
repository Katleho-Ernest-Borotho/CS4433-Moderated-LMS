# CS4433 — tagged Windows release candidates

This repository uses an immutable-release workflow to publish the two assessed source baselines as **GitHub pre-releases**.

| Tag | Source commit | Expected automated tests | Intended changes |
|---|---|---:|---|
| `v1.0` | `0f5ae44003fbea1583595a1afc8b59a376b2e64c` | 115 | Four-role moderated offline LMS |
| `v1.1` | `adbd8e194fc0856e9e478880d385a1b1c14d0a5e` | 128 | CR1 whole-class assignment deadline extensions |

The v1.1 source SHA is the exact SHA recorded in the independently tested Windows package provenance. The v1.0 SHA is the preserved pre-CR1 source/documentation baseline; the original local v1.0 package did not record an exact SHA. Newly published Windows ZIPs are therefore **fresh CI builds**, not copies of the previously tested local folders.

## Publishing

1. In the repository, click **Actions**.
2. Select **Publish Windows release candidate (v1.0 or v1.1)**.
3. Click **Run workflow** on `main`, choose `v1.0`, and run.
4. Review the job: checkout SHA, build, expected CTest count, 100% passing tests, complete Qt/UCRT64 portable ZIP, checksum, release notes.
5. Once it succeeds, run the same workflow for `v1.1`.
6. Open **Releases** to see both versions with ZIP assets and **Tags** to see the immutable source snapshots.

The workflow uses a Windows GitHub runner to package the selected commit via that commit's own `scripts/package_windows_ucrt64.sh`, writes `RELEASE_PROVENANCE.txt`, archives an entire portable folder (not just an EXE), calculates SHA-256, and publishes as a **pre-release**. It refuses to replace an existing published release. **Do not rerun publication to try to overwrite a tag or artifact**.

These are not signed installers. No school database, private accounts, live submissions or credentials should be bundled. Check Qt/MinGW/libsodium licence obligations and acceptance tests before treating a candidate as production-approved. School data lives under the per-user Windows application-data directory.

The GitHub **Packages** panel is not used: a desktop ZIP belongs under **Releases**. Source archives are automatically provided for each tag. The pre-release status is intentional because formal supervisor sign-off is not recorded.

[Release index](../../releases) · [CI status](../../actions)
