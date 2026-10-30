#!/usr/bin/env python3
"""Static consistency checks for the Group II OMER design package.

This script checks document arithmetic and required safety/competition gates.
It does not certify rule compliance, structural strength, or hardware operation.
"""
from pathlib import Path
import re
ROOT = Path(__file__).resolve().parents[1]

def read(name):
    return (ROOT / name).read_text(encoding="utf-8")

bom = read("04_BOM_BUDGET.md")
amounts = [int(x) for x in re.findall(r"\| 1 套?\s*\|\s*(\d+)\s*\|", bom)]
# Read the explicitly listed USD estimates from the table, excluding total row.
rows = re.findall(r"^\| [^|]+ \| [^|]+ \| [^|]+ \| (\d+) \|", bom, re.M)
cost = sum(map(int, rows))
assert cost == 145, f"BOM estimate changed: expected 145 USD, got {cost}"
assert cost <= 165, "BOM exceeds the problem cost cap"

runbook = read("05_SHOW_RUNBOOK.md")
segments = re.findall(r"\| (\d):(\d{2})–(\d):(\d{2}) \|", runbook)
assert len(segments) == 10, f"Expected 10 timed stages, got {len(segments)}"
previous = 0
for h1, m1, h2, m2 in segments:
    start = int(h1) * 60 + int(m1)
    end = int(h2) * 60 + int(m2)
    assert start == previous, f"Timeline gap/overlap at {h1}:{m1}"
    assert end > start, "Non-positive stage duration"
    previous = end
assert previous == 7 * 60 + 15, f"Target runtime should be 7:15, got {previous // 60}:{previous % 60:02d}"
assert previous < 8 * 60, "Show plan exceeds 8-minute cap"

requirements = read("01_REQUIREMENTS_SCORE.md")
for token in ("B7a", "B7g1–5", "B8", "B10", "B11", "B12/H", "F"):
    assert token in requirements, f"Missing rule traceability token: {token}"

system = read("02_SYSTEM_DESIGN.md")
for token in ("手摇", "脚控", "独立", "官方澄清", "双保险销"):
    assert token in system, f"Missing system design/safety concept: {token}"

print("PASS: BOM estimate = $145 / $165 cap")
print("PASS: show timeline = 7:15 / 8:00 cap")
print("PASS: key rule traceability and safety concepts present")
print("NOTE: This is a static document gate, not a mechanical or competition certification.")
