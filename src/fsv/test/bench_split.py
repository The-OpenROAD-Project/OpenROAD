#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
"""Groups a perf profile of fsv into the stages of a walk.

Record a profile of BenchSuite (or of openroad running a solve), then:

  perf report -i perf.data --no-children --sort symbol --stdio \\
      | python3 bench_split.py

Each line of the report is assigned to the first stage whose pattern
matches its symbol; the shares are printed in percent of all samples.
"""

import re
import sys

# The first match wins, so more specific patterns come first.
STAGES = [
    ("table build", r"tabulate|LayerModes|modeTerms|zModes|AliasTable::AliasTable"),
    ("reading the design", r"odb::|lefin|defin|NetWindow|readNetWindow"),
    ("shell candidates", r"listShells|forEachOverlapping"),
    ("dielectric planes", r"Dielectric<"),
    ("nearest conductor", r"SpatialIndex<\d>::nearest"),
    (
        "table sampling",
        r"AliasTable|sampleTransition|sampleFirstHop|cubeFacePoint"
        r"|squareEdgePoint|LayeredTableCache|mixture",
    ),
    ("random numbers", r"Philox"),
    (
        "start points",
        r"startPoint|exposed|makeGaussianSurface|mergeBoxes|boxOffset",
    ),
    ("walk loop", r"runRound|Walker"),
]
LINE = re.compile(r"^\s*([\d.]+)%\s+\[\S+\]\s+(.*)$")


def main():
    shares = {name: 0.0 for name, _ in STAGES}
    shares["other"] = 0.0
    for line in sys.stdin:
        m = LINE.match(line)
        if not m:
            continue
        share, symbol = float(m.group(1)), m.group(2)
        for name, pattern in STAGES:
            if re.search(pattern, symbol):
                shares[name] += share
                break
        else:
            shares["other"] += share
    for name, share in sorted(shares.items(), key=lambda item: -item[1]):
        print(f"{name:20s} {share:5.1f}%")


if __name__ == "__main__":
    main()
