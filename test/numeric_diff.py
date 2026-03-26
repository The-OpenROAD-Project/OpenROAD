#!/usr/bin/env python3
"""Diff two files with floating-point tolerance (1% relative).

Floats (numbers with '.' or 'e/E') are compared with tolerance.
Plain integers (message IDs, counts) are compared exactly.
Exit 0 if match, 1 if different.
"""

import math, re, sys

FLOAT_RE = re.compile(r"[+-]?(?:\d+\.\d*|\.\d+)(?:[eE][+-]?\d+)?|[+-]?\d+[eE][+-]?\d+")

def lines_match(a, b):
    if a == b:
        return True
    if FLOAT_RE.split(a) != FLOAT_RE.split(b):
        return False
    nums_a, nums_b = FLOAT_RE.findall(a), FLOAT_RE.findall(b)
    return len(nums_a) == len(nums_b) and all(
        math.isclose(float(x), float(y), rel_tol=1e-2, abs_tol=1e-12)
        for x, y in zip(nums_a, nums_b))

if __name__ == "__main__":
    lines1 = open(sys.argv[1]).readlines()
    lines2 = open(sys.argv[2]).readlines()
    diffs = [(i+1, l1, l2)
             for i, (l1, l2) in enumerate(zip(lines1, lines2))
             if not lines_match(l1.rstrip("\n"), l2.rstrip("\n"))]
    if len(lines1) != len(lines2):
        diffs.append((min(len(lines1), len(lines2))+1, "<file lengths differ>", ""))
    for n, l1, l2 in diffs:
        print(f"{n}c{n}\n< {l1}\n---\n> {l2}")
    sys.exit(1 if diffs else 0)
