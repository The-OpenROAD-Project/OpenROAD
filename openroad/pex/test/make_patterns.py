#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
"""Writes patterns.def: the experiments' patterns laid out on sky130hd.

Each case sits in its own 40 µm cell, so that a 10 µm halo around one
case's nets takes in nothing of another. Every case appears twice: as
drawn along the bottom row (wires on met2 vertical, covers and crossings
on met3 horizontal, both preferred) and rotated by 90 degrees in the row
above, its nets named with a "_rot" suffix, where every wire runs in its
layer's non-preferred direction and each via is replaced by its rotated
"_R" variant. On a planar stack a net and its rotated copy have the same
capacitance.

    bazelisk build //openroad/pex/test:patterns_def
"""

import sys

CELL = 40.0  # µm
MET2_PITCH = 0.28  # minimum width plus minimum spacing
ROTATED_VIA = {
    "M1M2_PR": "M1M2_PR_R",
    "M2M3_PR": "M2M3_PR_R",
    "M3M4_PR": "M3M4_PR_R",
}


class Case:
    """Nets in cell coordinates, µm. A wire is (layer, points, via at
    the last point or None); a plate is (layer, width, start, end)."""

    def __init__(self, name):
        self.name = name
        self.nets = {}  # name -> (nondefault rule or None, [wire])
        self.plates = []

    def wire(self, net, layer, points, via=None, rule=None):
        self.nets.setdefault(net, (rule, []))[1].append((layer, points, via))
        return self

    def plate(self, layer, width, start, end):
        self.plates.append((layer, width, start, end))
        return self


def cases():
    out = []
    # e1: a wire whose cover and neighbor change along it.
    c = Case("e1")
    c.wire("e1_v", "met2", [(5, 0), (5, 10)])
    c.wire("e1_n", "met2", [(5 + MET2_PITCH, 6.6), (5 + MET2_PITCH, 10)])
    c.plate("met3", 5, (2, 2.5), (8, 2.5))
    out.append(c)
    # e2: crossing arrays at coverage 0.25 and 0.5.
    for name, pitch, count in (("e2a", 1.2, 7), ("e2b", 0.6, 13)):
        c = Case(name)
        c.wire(f"{name}_v", "met2", [(5, 0), (5, 10)])
        for k in range(count):
            y = 5 + (k - (count - 1) / 2) * pitch
            c.wire(f"{name}_c{k}", "met3", [(3, y), (7, y)])
        out.append(c)
    # e3: lone short wires, and two line ends facing at a 0.3 µm gap
    # (the drawn ends extend half the width past the points).
    for length in (0.5, 1, 2, 5):
        name = f"e3_l{length:g}".replace(".", "p")
        c = Case(name)
        c.wire(f"{name}_v", "met2", [(5, 0), (5, length)])
        out.append(c)
    c = Case("e3_face")
    c.wire("e3_face_a", "met2", [(5, 0), (5, 3)])
    c.wire("e3_face_b", "met2", [(5, 3.44), (5, 6.44)])
    out.append(c)
    # e4: three neighbors on each side at minimum spacing, and a wide
    # neighbor.
    c = Case("e4")
    c.wire("e4_v", "met2", [(5, 0), (5, 10)])
    for k in (1, 2, 3):
        for side, sign in (("l", -1), ("r", 1)):
            x = 5 + sign * k * MET2_PITCH
            c.wire(f"e4_{side}{k}", "met2", [(x, 0), (x, 10)])
    out.append(c)
    c = Case("e4_wide")
    c.wire("e4_wide_v", "met2", [(5, 0), (5, 10)])
    c.wire("e4_wide_n", "met2", [(5.71, 0), (5.71, 10)], rule="WIDE")
    out.append(c)
    # e6: different spacings on the two sides.
    c = Case("e6")
    c.wire("e6_v", "met2", [(5, 0), (5, 10)])
    c.wire("e6_l", "met2", [(5 - MET2_PITCH, 0), (5 - MET2_PITCH, 10)])
    c.wire("e6_r", "met2", [(5.56, 0), (5.56, 10)])
    out.append(c)
    # e8: a via stack at the wire's end and a via from met1 in its middle,
    # a neighbor at one pitch.
    c = Case("e8")
    c.wire("e8_v", "met2", [(5, 0), (5, 10)], via="M2M3_PR")
    c.wire("e8_v", "met3", [(5, 10)], via="M3M4_PR")
    c.wire("e8_v", "met1", [(5, 5)], via="M1M2_PR")
    c.wire("e8_n", "met2", [(5.46, 0), (5.46, 10)])
    out.append(c)
    # A jog: a wire turning onto its layer's non-preferred direction, with a
    # neighbor along the turned part.
    c = Case("jog")
    c.wire("jog_v", "met2", [(5, 0), (5, 5), (9, 5)])
    c.wire("jog_n", "met2", [(5.5, 5 + MET2_PITCH), (9, 5 + MET2_PITCH)])
    out.append(c)
    return out


def dbu(v):
    return round(v * 1000)


def place(case, index, rotated, height):
    """The case's points in die coordinates: cell `index` of the bottom
    row, or its 90-degree rotation (x, y) -> (height - y, x) into the row
    above."""
    ox = (index + 0.5) * CELL - 5
    oy = CELL / 2 - 5

    def at(p):
        x, y = p[0] + ox, p[1] + oy
        return (height - y, x) if rotated else (x, y)

    return at


def write(out):
    all_cases = cases()
    width = CELL * len(all_cases)
    height = CELL
    # The rotated row lands at x in [0, height) and y in [0, width), so
    # it is moved up by one row's height to sit above the bottom row.
    die = (width, height + width)
    nets = []
    plates = []
    for index, case in enumerate(all_cases):
        for rotated in (False, True):
            at = place(case, index, rotated, height)

            def move(p, at=at, rotated=rotated):
                x, y = at(p)
                return (x, y + height) if rotated else (x, y)

            suffix = "_rot" if rotated else ""
            for name, (rule, wires) in case.nets.items():
                nets.append((name + suffix, rule, wires, move, rotated))
            for layer, w, start, end in case.plates:
                plates.append((layer, w, move(start), move(end)))

    out.write(
        "VERSION 5.8 ;\n"
        'DIVIDERCHAR "/" ;\n'
        'BUSBITCHARS "[]" ;\n'
        "DESIGN patterns ;\n"
        "UNITS DISTANCE MICRONS 1000 ;\n"
        f"DIEAREA ( 0 0 ) ( {dbu(die[0])} {dbu(die[1])} ) ;\n"
        "NONDEFAULTRULES 1 ;\n"
        "    - WIDE\n"
        "      + LAYER li1 WIDTH 170 SPACING 170\n"
        "      + LAYER met1 WIDTH 140 SPACING 140\n"
        "      + LAYER met2 WIDTH 1000 SPACING 140\n"
        "      + LAYER met3 WIDTH 300 SPACING 300\n"
        "      + LAYER met4 WIDTH 300 SPACING 300\n"
        "      + LAYER met5 WIDTH 1600 SPACING 1600\n"
        "    ;\n"
        "END NONDEFAULTRULES\n"
    )
    out.write("SPECIALNETS 1 ;\n    - VSS + USE GROUND\n")
    for i, (layer, w, start, end) in enumerate(plates):
        lead = "      + ROUTED" if i == 0 else "      NEW"
        out.write(
            f"{lead} {layer} {dbu(w)} + SHAPE STRIPE "
            f"( {dbu(start[0])} {dbu(start[1])} ) "
            f"( {dbu(end[0])} {dbu(end[1])} )\n"
        )
    out.write("    ;\nEND SPECIALNETS\n")
    out.write(f"NETS {len(nets)} ;\n")
    for name, rule, wires, move, rotated in nets:
        out.write(f"    - {name} + USE SIGNAL\n")
        if rule:
            out.write(f"      + NONDEFAULTRULE {rule}\n")
        for i, (layer, points, via) in enumerate(wires):
            lead = "      + ROUTED" if i == 0 else "      NEW"
            text = " ".join(
                f"( {dbu(x)} {dbu(y)} )" for x, y in (move(p) for p in points)
            )
            if via:
                text += " " + (ROTATED_VIA[via] if rotated else via)
            out.write(f"{lead} {layer} {text}\n")
        out.write("    ;\n")
    out.write("END NETS\nEND DESIGN\n")


if __name__ == "__main__":
    write(sys.stdout)
