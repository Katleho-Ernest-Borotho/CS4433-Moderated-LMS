#!/usr/bin/env python3
"""Prepare the two official CS4433 hand-in artefacts without school data."""
from __future__ import annotations
from pathlib import Path
import hashlib, sys, shutil, subprocess, datetime

REPO = "https://github.com/Katleho-Ernest-Borotho/CS4433-Moderated-LMS"
ROOT = Path(__file__).resolve().parents[1]
def die(message):
    print("ERROR:", message, file=sys.stderr)
    sys.exit(2)
if len(sys.argv) != 2:
    die("Usage: python scripts/assemble_final_submission.py /path/to/final_report.pdf")
src = Path(sys.argv[1]).expanduser().resolve()
if not src.is_file() or src.suffix.lower() != ".pdf":
    die("Provide the existing FINAL report PDF (not the Word or Markdown file)")
if src.read_bytes()[:5] != b"%PDF-":
    die("Input is not a PDF")
output = ROOT/"final-submission-output"
output.mkdir(exist_ok=True)
dst = output/"CS4433_Final_Report_202322762.pdf"
if src != dst:
    shutil.copy2(src,dst)
sha=hashlib.sha256(dst.read_bytes()).hexdigest()
try:
    commit=subprocess.check_output(["git","rev-parse","HEAD"],cwd=ROOT,
                                   text=True).strip()
except Exception:
    commit="not available"
(output/"GIT_REPOSITORY_LINK.txt").write_text(REPO+"\n",encoding="utf-8")
(output/"HANDIN_CHECKSUMS.txt").write_text(
    f"Report SHA-256: {sha}\n"
    f"Repository link: {REPO}\n"
    f"Repository HEAD when staged: {commit}\n"
    f"Staged UTC: {datetime.datetime.now(datetime.timezone.utc).isoformat()}\n"
    "NB: Packaged v1.1 binary provenance is separately recorded in Phase 8.\n",
    encoding="utf-8")
print("Official hand-in artefacts prepared:")
print(" PDF:",dst)
print(" Repository:",REPO)
print(" SHA-256:",sha)
print("Supplementary staging metadata:",output/"HANDIN_CHECKSUMS.txt")
print("Do not submit extra files unless the lecturer asks for them.")
