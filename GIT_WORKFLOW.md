# GitHub setup and everyday workflow

## 1. Create the remote repository

Log in to https://github.com/new and create a repository with the name `CS4433-Moderated-LMS`.

- Visibility: **Private** (recommended while submitted PDFs include personal details).
- **Do not** select “Add a README file”, “Add .gitignore”, or “Choose a license” during creation. The local starter already includes README and .gitignore.
- Copy the repository's HTTPS URL from GitHub's Quick Setup page.

## 2. Initialize and push this local folder

Open a terminal (Kali/Linux terminal or Git Bash on Windows). Change into the extracted `CS4433-Moderated-LMS` directory, then:

```bash
git init -b main
git add .
git commit -m "docs: add phase 1 and phase 2 submitted baselines"
git remote add origin https://github.com/YOUR_USERNAME/CS4433-Moderated-LMS.git
git push -u origin main
```

Replace `YOUR_USERNAME` with your GitHub username. If Git asks for your commit identity, configure **your actual name and a verified GitHub email or GitHub no-reply email**:

```bash
git config --global user.name "YOUR_NAME"
git config --global user.email "YOUR_GITHUB_EMAIL"
```

Then rerun `git commit` and `git push`. Sign in using GitHub's supported authentication flow (e.g. credential manager or SSH); do not put a password or token in the repository URL or chat.

## 3. Everyday changes

After editing files, from the repo folder:

```bash
git status
git add .
git commit -m "docs: update phase 3 architecture design"
git push
```

`git commit` records a local snapshot. `git push` uploads that snapshot to GitHub. You can **commit offline** and push later when Internet access returns.

## 4. Working with feature branches

Use `main` for the reviewed baseline. For a substantial design or code change:

```bash
git switch -c feature/phase3-architecture
# Edit/add files...
git add .
git commit -m "docs: draft architecture and component diagram"
git push -u origin feature/phase3-architecture
```

Review the branch and merge into `main` via a GitHub pull request when ready. Pull the latest `main` before starting the next change. For a small individual project, keep the branch strategy lightweight but document it.

## 5. Tag actual releases later

Do not tag an empty documentation-only repository as `v1.0`. After the Week 6 deployment package is ready and validated:

```bash
git tag -a v1.0 -m "LMS initial deployment release"
git push origin v1.0
```

After maintenance Change Request CR1 is implemented and regression-tested in Week 7:

```bash
git tag -a v1.1 -m "LMS maintenance release with class extensions"
git push origin v1.1
```

## 6. Avoid accidental commits

Review `git status` and `git diff --staged` before every commit. `.gitignore` excludes local SQLite databases, build outputs and common secrets, but it is not a substitute for reviewing what is staged.

## 7. CI plan

The assignment requires CI to build the project and run automated tests on pushes. Add `.github/workflows/ci.yml` **when the CMake project and tests actually exist**. Do not claim that CI has run while the repository contains documents only.
