# Moderated Learning Management System — Role-Based User Manual (v1.1)

**System type:** Offline Windows desktop LMS, single shared school computer  
**Release:** v1.1 CR1 — whole-class assignment deadline extensions  
**Audience:** School Administrator, Lecturer, Head of Department (HoD), Student

## Getting started

Open `ModeratedLMS.exe` from the complete extracted deployment folder, without moving or deleting `lms_schema.sql` or its DLL/plugin subfolders. A new installation with no user accounts presents **Initial administrator setup**. The first Administrator enters a username (3–40 permitted characters) and password of at least 12 characters. The Administrator should use a unique, confidential password and keep the recovery procedure securely with the school's IT officer.

After setup, normal users sign in through **Log in**. The dashboard displays tabs appropriate to their role. Use **Log out** before handing the shared computer to another user. This is a local application, not a web or mobile app; Internet access is not required during normal operation.

**Safeguard:** This system stores learner work, results and password hashes locally. Use a private Windows account where feasible, never leave an Administrator session open, and use only demonstration records during acceptance testing. Make verified backups before any migration or restore.

## A. Administrator manual

After logging in, open **User management** to create users. Supply a permitted username, a temporary strong password and role (Administrator, Lecturer, Student or HoD). Choose **Create user**; use **Refresh users** to confirm the record appears. To change a username, select its row and click **Rename selected**. To block access, use **Deactivate selected**; later use **Reactivate selected** to restore the account. Use **Reset password** for a selected non-current user when the original password has been forgotten; passwords are replaced, not displayed.

Next open **Academic management**. Create the required department under **Departments**, then create a course under **Courses** and associate it with its department. Use **Role and course allocations** to assign an HoD to a department, a Lecturer to a course and a Student to a course. Sign out and sign in as the users to confirm their assigned/enrolled courses appear. If a course is missing, check both the role assignment and the course/dept selection.

Open **Maintenance and reports** to review counts of users, departments, courses, assignments, submissions and released results. Press **Refresh report** before exporting. Choose **Export administrative CSV...** to save aggregate statistics; this export does not contain passwords or individual submitted text.

**Back up data:** Click **Create full backup...** and select a protected folder *outside* the LMS application-data directory. The backup is a folder containing `lms.sqlite3`, `attachments/files`, and `manifest.txt` with checksums. Then use **Verify backup folder...** and choose the generated backup folder. Backup files contain private information and are not encrypted by the application.

**Restore:** Close the LMS completely. Only then run the separate `ModeratedLMSRestore.exe`. Select a verified backup and type the required confirmation. The tool makes a safety copy of the current data before replacement. A restore replaces the current records with the selected snapshot; confirm with the IT officer before executing on a real installation.

## B. Lecturer manual

Log in as a Lecturer who has been assigned to the course by the Administrator. Open **My courses** to see assigned courses. In **Assignments and extensions**, select the course and choose **Create assignment**. Enter the assignment title, deadline and maximum mark, then create it. Due dates appear in local time; stored dates use UTC.

For a special accommodation to one enrolled Student, choose **Individual deadline extension (Lecturer)**, select the assignment and Student, enter a later deadline, then click **Grant individual extension**. The Student's effective due date changes without changing other Students' dates.

**New in v1.1:** Choose **Whole-class deadline extension (Lecturer)**, select the assignment and a later date, then click **Grant whole-class extension** and confirm. Every enrolled Student receives that class due date, but any longer approved individual extension remains in force. Repeating a whole-class extension must move the date forward. The system refuses to change a class deadline after marking has begun for that assignment; this prevents changing the basis of stored penalties.

Open **Marking** to select your course, assignment and the submitted Student's latest version. Review the typed work; use **Save latest attached file...** if an attachment is present. Enter the earned mark and written feedback, then select **Save provisional mark**. The system calculates the late penalty automatically. A revision to an existing mark must contain a reason so that audit information is retained. **A provisional mark is not yet visible to the Student.** Only the appropriate HoD can approve and release it.

Late penalty policy: first late work is accepted. A commenced late day costs 10% of the assignment maximum mark, for up to three days; work more than 72 hours late receives a final mark of zero. Student resubmission is allowed only on or before the effective deadline; old versions remain recorded.

## C. Head of Department manual

Sign in as HoD assigned to the correct department. Open **Results and moderation**. Choose the departmental course and inspect the assigned Lecturer's provisional marking, saved work and feedback. If needed, save a review copy of an attached file.

Use **Approve moderation** once the submission and mark satisfy the department's rules. If you adjust a numerical mark, provide a reason: the application records who changed it, what changed and when. Approval and release are separate actions. Only after the effective deadline and appropriate moderation should you select **Release approved result**. Students cannot see provisional or merely approved marks before release.

Do not attempt to moderate another department's work. A class extension into the future also postpones the earliest permitted moderation/release. If a release action fails, check the selected result, approval state and Student's effective deadline.

## D. Student manual

Sign in with the username and password provided by the Administrator. Open **My courses** to see the courses for which you are enrolled. Open **Assignments and extensions** and select a course to see its assignments, original due time, **Class due** time and **Your due** time.

**Class due** is the latest whole-class extension, if any. **Your due** is the Student-specific effective deadline: the latest of the original deadline, whole-class extension and any approved individual extension. If the Lecturer gave the Student a longer individual extension, **Your due** remains later even when the class deadline changes.

To submit work, select the assignment under **Submit assignment (Student)**. Type an answer and/or choose an attachment. Accepted attachments are PDF, DOCX or TXT and must be between 1 byte and 10 MiB. Press **Submit / Resubmit** and check that a new version appears. If correcting a mistake before the effective deadline, submit again; earlier versions remain saved. A first attempt can still be submitted late, but a late replacement is not permitted.

After the Lecturer marks an attempt, the Student will still not see that result until the HoD has explicitly released it. Open **Results and moderation** (Student's results view) or **My released results**, then click **Refresh released results** to view released marks and feedback.

## E. Troubleshooting and responsible operation

| Symptom | Likely issue and next step |
|---|---|
| Login rejected | Check username/password and whether the account is active; ask Administrator for a password reset |
| Missing course | Ask Administrator to verify Lecturer assignment / Student enrolment |
| Cannot resubmit | Effective deadline has passed; original first late submission is still permitted |
| Cannot grant class extension | New time is not later, Lecturer lacks course permission, or marking already started |
| No mark visible | Lecturer mark may be provisional or HoD has not explicitly released it |
| Files missing after restore | Verify that the backup included both SQLite and `attachments/files`; stop before overwriting anything further |
| Program will not open | Keep the EXE, schema, Qt DLLs and `platforms` subdirectory together; record exact Windows error |
| Another instance detected | Close LMS and any Restore/Recovery tools before restarting |

**Data protection and fairness:** Do not use shared logins, publish CSV exports or backup folders containing learner information, bypass HoD approval, or change an assessment without an auditable reason. Discuss unusual penalties or extensions with the school rather than silently altering stored results.

## F. Where school data is stored

The app uses `QStandardPaths::AppLocalDataLocation` for organisation `CS4433` and application `ModeratedLMS`. Within that Windows per-user application-data area, the application creates `lms.sqlite3`, `attachments/` and a lock file. The portable release folder contains executables and dependencies, *not* the actual school database. Always create a verified full backup before moving between major releases.
