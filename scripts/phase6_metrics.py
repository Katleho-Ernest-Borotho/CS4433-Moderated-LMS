#!/usr/bin/env python3
"""Dependency-free C++ source metrics for CS4433. Run at repository root."""
from __future__ import annotations
from pathlib import Path
from datetime import datetime, timezone
from collections import defaultdict
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "docs/phase-6-configuration-qa/MEASURED_CODE_METRICS.md"
SCOPES = ("src/application", "src/domain", "src/persistence",
          "src/security", "src/ui", "src/tools", "src", "include", "tests")
rows = []
summary = defaultdict(lambda: [0, 0, 0])
for group in SCOPES:
    for path in (ROOT / group).rglob("*"):
        if group == "src" and path.parent != ROOT / "src":
            continue
        if not path.is_file() or path.suffix not in {".cpp", ".h", ".hpp"}:
            continue
        contents = path.read_text(encoding="utf-8", errors="replace").splitlines()
        nonblank = sum(bool(x.strip()) for x in contents)
        # Approximation: exclude blank lines and entire-line C++ comments;
        # block comments and multiline C++ string literals require a parser.
        approx = sum(bool(x.strip()) and not x.lstrip().startswith(("//","*","/*","*/"))
                     for x in contents)
        rows.append((path.relative_to(ROOT).as_posix(), len(contents), nonblank, approx))
        summary[group][0] += 1
        summary[group][1] += len(contents)
        summary[group][2] += approx

try:
    sha = subprocess.check_output(["git","rev-parse","HEAD"], cwd=ROOT, text=True).strip()
except Exception:
    sha = "not available"

lines = [
    "# CS4433 — Measured C++ Source Metrics",
    "",
    f"Generated at (UTC): {datetime.now(timezone.utc).strftime('%Y-%m-%d %H:%M:%S')}",
    f"Git commit: `{sha}`",
    "",
    "## Files and lines by code layer",
    "",
    "| Layer | Source/header files | Physical lines | Approx. noncomment nonblank lines |",
    "|---|---:|---:|---:|"
]
for key in SCOPES:
    v=summary[key];lines.append(f"| `{key}` | {v[0]} | {v[1]} | {v[2]} |")
lines+=["",f"**Totals:** {sum(x[0] for x in summary.values())} files, "
        f"{sum(x[1] for x in summary.values())} physical lines. ",
        "",
        "## Ten longest source files","",
        "| File | Physical lines | Nonblank lines | Approx. code lines |",
        "|---|---:|---:|---:|"]
for name,pl,nb,ap in sorted(rows,key=lambda x:x[1],reverse=True)[:10]:
    lines.append(f"| `{name}` | {pl} | {nb} | {ap} |")
lines+=["","## Method complexity interpretation","",
"These are **physical line counts**, not McCabe cyclomatic complexity.",
"Comments are filtered approximately. To obtain accurate per-function",
"cyclomatic complexity, use a dedicated complexity analyser and record",
"its version, invocation and source SHA in a separate file.",
"",
"**Review recommendation:** Inspect the longest repository/service",
"functions, especially submission transactions, moderation audit writes,",
"backup/restore staging and the role-specific Qt panels.",
"",
"Do not report measured complexity figures without actually executing",
"an analysis tool."]
OUT.parent.mkdir(parents=True, exist_ok=True)
OUT.write_text("\n".join(lines)+"\n",encoding="utf-8")
print(f"Metrics report: {OUT}")
print(f"{len(rows)} C++ headers/source files; {sum(x[1] for x in summary.values())} physical lines.")
