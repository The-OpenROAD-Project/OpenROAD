#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
"""Reference capacitances for 3D fsv golden structures, from FasterCap.

FasterCap is a boundary-element capacitance solver (LGPL-2.1). It is not a
build dependency; point FSV_FASTERCAP at its binary to regenerate:

  FSV_FASTERCAP=/path/to/FasterCap \
      python3 fastercap_golden.py crossing.txt > fastercap.txt

Cross sections use fv2d_golden.py instead, which is far more accurate.

Each structure file uses the fsv test format (see crossing.txt). fsv has an
infinite ground plane and infinite planar layers; FasterCap needs finite
ones, so the model here truncates both to a square of half-width L:
- a ground plate at z = 0, embedded in the bottom layer's material, which is
  extended below the plate so that the plate sits in a single medium;
- one dielectric sheet per planar interface, plus the slabs' side faces
  against the background and the extended bottom face.

Limits of the reference: tighter settings are not affordable (half-width 10 µm
at -a1e-3, or 20 µm at any accuracy, runs for more than 10 minutes per
structure), so the values carry FasterCap's own error:
- dielectric discretization: with dielectric sheets, totals come out about
  1.5% above fsv on structures where fsv matches the finite-volume
  reference within 0.1%, and tighter -a settings do not reduce this;
- truncation: flux escaping past the plate's edge is missing from the ground
  entries, by roughly the wire height over L (about 10% for a wire 1.2 µm
  up at L = 10 µm), so compare the total minus the couplings instead;
- the matrix is asymmetric by a few percent at -a2e-3.
The mesh is checked at half-width 5 µm, where -a2e-3 and -a1e-3 must agree.
"""

import os
import re
import subprocess
import sys
import tempfile

# FasterCap stops refining at this relative error.
ACCURACY = 2e-3
# Truncation half-width in µm for the reference values.
HALF_WIDTH = 10.0
# Mesh check: at this half-width, -a ACCURACY and -a ACCURACY / 2 must agree
# within AGREEMENT of each total.
CHECK_HALF_WIDTH = 5.0
AGREEMENT = 5e-3
# Seconds before a FasterCap run is abandoned.
TIMEOUT = 900
# FasterCap reads µm coordinates as metres: F -> fF for µm geometry.
TO_FF = 1e9


def read_structure(path):
    layers, boxes, background = [], [], None
    with open(path, encoding="utf-8") as f:
        for line in f:
            words = line.split("#")[0].split()
            if not words:
                continue
            if words[0] == "background":
                background = float(words[1])
            elif words[0] == "layer":
                layers.append((float(words[1]), float(words[2])))
            elif words[0] == "box":
                boxes.append((words[1], [float(v) for v in words[2:8]]))
            else:
                raise ValueError(f"{path}: unknown line {line.strip()}")
    return background, layers, boxes


def eps_at(z, layers, background):
    for top, eps in layers:
        if z < top:
            return eps
    return background


def quad(name, x0, y0, z0, x1, y1, z1, ref=None):
    """One axis-aligned rectangle (one coordinate pair is degenerate)."""
    if z0 == z1:
        corners = [(x0, y0, z0), (x1, y0, z0), (x1, y1, z0), (x0, y1, z0)]
    elif y0 == y1:
        corners = [(x0, y0, z0), (x1, y0, z0), (x1, y0, z1), (x0, y0, z1)]
    else:
        corners = [(x0, y0, z0), (x0, y1, z0), (x0, y1, z1), (x0, y0, z1)]
    text = f"Q {name} " + "  ".join(f"{x:g} {y:g} {z:g}" for x, y, z in corners)
    if ref:
        text += "  " + " ".join(f"{v:g}" for v in ref)
    return text + "\n"


def box_panels(name, b):
    x0, y0, z0, x1, y1, z1 = b
    return "".join(
        [
            quad(name, x0, y0, z0, x1, y1, z0),
            quad(name, x0, y0, z1, x1, y1, z1),
            quad(name, x0, y0, z0, x1, y0, z1),
            quad(name, x0, y1, z0, x1, y1, z1),
            quad(name, x0, y0, z0, x0, y1, z1),
            quad(name, x1, y0, z0, x1, y1, z1),
        ]
    )


def fastercap_input(background, layers, boxes, half_width):
    """A FasterCap single-file input, and the conductor names in order."""
    L = half_width
    plate = 0.1  # ground plate thickness
    floor = -0.5  # bottom of the extended bottom layer
    sections = []
    main = ["* fsv golden structure\n"]

    # Ground plate inside the (extended) bottom layer.
    sections.append(("ground.txt", box_panels("ground", (-L, -L, -plate, L, L, 0))))
    main.append(f"C ground.txt {layers[0][1]:g} 0 0 0\n")

    # Wires, each in the medium around it.
    names = ["ground"]
    for i, (name, b) in enumerate(boxes):
        z_mid = 0.5 * (b[2] + b[5])
        eps = eps_at(z_mid, layers, background)
        for z in (b[2], b[5]):
            if eps_at(z + 1e-9 if z == b[2] else z - 1e-9, layers, background) != eps:
                raise ValueError(f"box {name} crosses a planar interface")
        sections.append((f"wire{i}.txt", box_panels(name, b)))
        main.append(f"C wire{i}.txt {eps:g} 0 0 0\n")
        names.append(name)

    # Horizontal interfaces between layers, and between the top layer and
    # the background. The reference point is above each sheet: outperm is the
    # permittivity above.
    bottom = floor
    for i, (top, eps) in enumerate(layers):
        above = layers[i + 1][1] if i + 1 < len(layers) else background
        if above != eps:
            sections.append((f"sheet{i}.txt", quad("d", -L, -L, top, L, L, top)))
            main.append(f"D sheet{i}.txt {above:g} {eps:g} 0 0 0 0 0 {top + 1:g}\n")
        # Side faces of the slab against the background, with per-panel
        # reference points outside the slab.
        if eps != background:
            far = 2 * L
            sides = "".join(
                [
                    quad(
                        "d", -L, -L, bottom, L, -L, top, (0, -far, 0.5 * (bottom + top))
                    ),
                    quad("d", -L, L, bottom, L, L, top, (0, far, 0.5 * (bottom + top))),
                    quad(
                        "d", -L, -L, bottom, -L, L, top, (-far, 0, 0.5 * (bottom + top))
                    ),
                    quad("d", L, -L, bottom, L, L, top, (far, 0, 0.5 * (bottom + top))),
                ]
            )
            sections.append((f"sides{i}.txt", sides))
            main.append(
                f"D sides{i}.txt {background:g} {eps:g} 0 0 0 0 0 {floor - 1:g}\n"
            )
        bottom = top
    # The bottom face of the extended bottom layer, against the background.
    if layers[0][1] != background:
        sections.append(("floor.txt", quad("d", -L, -L, floor, L, L, floor)))
        main.append(
            f"D floor.txt {background:g} {layers[0][1]:g} 0 0 0 0 0 {floor - 1:g}\n"
        )

    text = "".join(main) + "End\n"
    for file_name, panels in sections:
        text += f"File {file_name}\n* panels\n{panels}End\n"
    return text, names


def run_fastercap(text, accuracy, timeout=None):
    exe = os.environ.get("FSV_FASTERCAP")
    if not exe:
        sys.exit("set FSV_FASTERCAP to the FasterCap binary")
    with tempfile.TemporaryDirectory() as tmp:
        path = os.path.join(tmp, "model.lst")
        with open(path, "w", encoding="utf-8") as f:
            f.write(text)
        out = subprocess.run(
            [exe, "-b", path, f"-a{accuracy:g}"],
            capture_output=True,
            text=True,
            cwd=tmp,
            check=False,
            timeout=timeout,
        ).stdout
    # The last "Dimension N x N" block is the final capacitance matrix.
    blocks = out.split("Dimension")
    if len(blocks) < 2:
        sys.exit("no capacitance matrix in FasterCap output:\n" + out[-2000:])
    rows = []
    for line in blocks[-1].splitlines()[1:]:
        words = line.split()
        if len(words) < 2 or not re.match(r"g\d+_", words[0]):
            if rows:
                break
            continue
        rows.append([float(v) for v in words[1:]])
    return rows


def solve(path):
    background, layers, boxes = read_structure(path)
    # Mesh check at the smaller half-width.
    text, names = fastercap_input(background, layers, boxes, CHECK_HALF_WIDTH)
    coarse = run_fastercap(text, ACCURACY, TIMEOUT)
    fine = run_fastercap(text, ACCURACY / 2, TIMEOUT)
    for i in range(1, len(names)):
        for j in range(len(names)):
            if abs(coarse[i][j] - fine[i][j]) > AGREEMENT * abs(fine[i][i]):
                sys.exit(
                    f"{path}: mesh check failed for {names[i]}/{names[j]}: "
                    f"{abs(coarse[i][j] - fine[i][j]) / abs(fine[i][i]):.2e} "
                    "of the total"
                )
    text, names = fastercap_input(background, layers, boxes, HALF_WIDTH)
    return names, run_fastercap(text, ACCURACY, TIMEOUT)


def main():
    print("# fsv golden capacitances from FasterCap 6.0.7, in fF.")
    print(f"# FasterCap -a{ACCURACY:g}; ground plate and layers truncated to")
    print(f"# half-width {HALF_WIDTH:g} µm. Mesh checked at half-width")
    print(
        f"# {CHECK_HALF_WIDTH:g} µm: -a{ACCURACY:g} and -a{ACCURACY / 2:g} agree within"
    )
    print(f"# {AGREEMENT:g} of the total. The values carry FasterCap's truncation")
    print("# and discretization error (see fastercap_golden.py).")
    print("# <structure> <master> <net> <capacitance>; cap[master] is the total,")
    print("# other entries are minus the coupling (Maxwell matrix).")
    for path in sys.argv[1:]:
        names, rows = solve(path)
        structure = os.path.splitext(os.path.basename(path))[0]
        for i in range(1, len(names)):
            for j in range(len(names)):
                print(f"{structure} {names[i]} {names[j]} {rows[i][j] * TO_FF:.6g}")


if __name__ == "__main__":
    main()
