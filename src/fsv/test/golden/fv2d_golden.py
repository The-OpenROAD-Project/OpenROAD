#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
"""Per-unit-length reference capacitances for fsv cross-section structures.

Solves the 2D Laplace equation in the y-z plane by finite volumes: a ground
plane at z = 0, planar layers, and wires infinitely long along x (the box x
extents are ignored). Needs numpy and scipy:

  python3 fv2d_golden.py etch_stop_pair.txt three_wires.txt > cross_section.txt

The grid is uniform with spacing h around the wires and grows geometrically
out to grounded far boundaries FAR µm away; wire faces and layer interfaces
lie on cell faces. Each value is extrapolated from h = 0.01, 0.005 and
0.0025 µm, assuming the error falls by a constant ratio per halving (the
corners make the order fractional, about 1.3). The header reports the largest
change from the finest grid to the extrapolated value.
"""

import sys

import numpy as np
import scipy.sparse as sp
import scipy.sparse.linalg as spla

EPS0 = 8.8541878128e-3  # fF/µm
SPACINGS = (0.01, 0.005, 0.0025)
# Uniform-grid margin around the wires, far-boundary distance (µm), and the
# geometric growth of cells beyond the margin.
MARGIN = 0.6
FAR = 40.0
GROWTH = 1.08


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


def grid_lines(breaks, lo, hi, h, far_lo, far_hi):
    """Lines spaced h on [lo, hi] plus every break, growing out to the far
    bounds."""
    lines = {round(v, 9) for v in np.arange(lo, hi + 1e-12, h)}
    lines |= {round(b, 9) for b in breaks}
    edge, step = lo, h
    while edge > far_lo:
        step *= GROWTH
        edge = max(edge - step, far_lo)
        lines.add(edge)
    edge, step = hi, h
    while edge < far_hi:
        step *= GROWTH
        edge = min(edge + step, far_hi)
        lines.add(edge)
    return np.array(sorted(lines))


def solve(background, layers, boxes, h):
    """The Maxwell matrix per unit length (fF/µm), nets in first-seen
    order, with the ground column last."""
    names = list(dict.fromkeys(name for name, _ in boxes))
    y_breaks = [v for _, b in boxes for v in (b[1], b[4])]
    z_breaks = [v for _, b in boxes for v in (b[2], b[5])]
    z_breaks += [top for top, _ in layers]
    y_lo, y_hi = min(y_breaks) - MARGIN, max(y_breaks) + MARGIN
    z_hi = max(z_breaks) + MARGIN
    Y = grid_lines(y_breaks, y_lo, y_hi, h, y_lo - FAR, y_hi + FAR)
    Z = grid_lines(z_breaks + [0.0], 0.0, z_hi, h, 0.0, z_hi + FAR)
    yc, zc = 0.5 * (Y[1:] + Y[:-1]), 0.5 * (Z[1:] + Z[:-1])
    dy, dz = np.diff(Y), np.diff(Z)
    ny, nz = len(yc), len(zc)

    def eps_at(z):
        for top, eps in layers:
            if z < top:
                return eps
        return background

    eps = np.repeat(np.array([eps_at(z) for z in zc])[None, :], ny, axis=0)
    # Cells inside a wire hold its net index; free cells hold -1.
    owner = -np.ones((ny, nz), dtype=int)
    for name, b in boxes:
        inside = (
            (yc[:, None] > b[1])
            & (yc[:, None] < b[4])
            & (zc[None, :] > b[2])
            & (zc[None, :] < b[5])
        )
        owner[inside] = names.index(name)
    free = owner < 0
    index = -np.ones((ny, nz), dtype=int)
    index[free] = np.arange(free.sum())
    n = int(free.sum())

    diag = np.zeros(n)
    rows, cols, vals = [], [], []
    # Conductance between each free cell and an adjacent wire cell: the wire
    # surface is the shared face.
    wire_cell, wire_net, wire_g = [], [], []

    def couple(a, b, area, half_a, half_b):
        """Faces between cell sets a and b (index tuples), given each side's
        half-width normal to the face."""
        fa, fb = free[a], free[b]
        both = fa & fb
        g = area[both] / (half_a[both] / eps[a][both] + half_b[both] / eps[b][both])
        ia, ib = index[a][both], index[b][both]
        np.add.at(diag, ia, g)
        np.add.at(diag, ib, g)
        rows.extend([ia, ib])
        cols.extend([ib, ia])
        vals.extend([-g, -g])
        for f_side, w_side, half in ((a, b, half_a), (b, a, half_b)):
            m = free[f_side] & ~free[w_side]
            g = area[m] / (half[m] / eps[f_side][m])
            np.add.at(diag, index[f_side][m], g)
            wire_cell.append(index[f_side][m])
            wire_net.append(owner[w_side][m])
            wire_g.append(g)

    I, J = np.meshgrid(np.arange(ny), np.arange(nz), indexing="ij")
    # Neighbors along y, then along z.
    a = (I[:-1, :], J[:-1, :])
    b = (I[1:, :], J[1:, :])
    couple(a, b, dz[J[:-1, :]], dy[I[:-1, :]] / 2, dy[I[1:, :]] / 2)
    a = (I[:, :-1], J[:, :-1])
    b = (I[:, 1:], J[:, 1:])
    couple(a, b, dy[I[:, :-1]], dz[J[:, :-1]] / 2, dz[J[:, 1:]] / 2)
    # Grounded boundaries: the plane at z = 0 and the far edges.
    for cells, area, half in (
        ((I[:, 0], J[:, 0]), dy, dz[0] / 2),
        ((I[:, -1], J[:, -1]), dy, dz[-1] / 2),
        ((I[0, :], J[0, :]), dz, dy[0] / 2),
        ((I[-1, :], J[-1, :]), dz, dy[-1] / 2),
    ):
        m = free[cells]
        g = area[m] / (half / eps[cells][m])
        np.add.at(diag, index[cells][m], g)

    rows = np.concatenate(rows + [np.arange(n)])
    cols = np.concatenate(cols + [np.arange(n)])
    vals = np.concatenate(vals + [diag])
    lu = spla.splu(sp.csc_matrix((vals, (rows, cols)), shape=(n, n)))
    wire_cell = np.concatenate(wire_cell)
    wire_net = np.concatenate(wire_net)
    wire_g = np.concatenate(wire_g)

    matrix = np.zeros((len(names), len(names) + 1))
    for master in range(len(names)):
        on = wire_net == master
        rhs = np.zeros(n)
        np.add.at(rhs, wire_cell[on], wire_g[on])
        phi = lu.solve(rhs)
        for net in range(len(names)):
            s = wire_net == net
            v = 1.0 if net == master else 0.0
            matrix[master, net] = EPS0 * np.sum(wire_g[s] * (v - phi[wire_cell[s]]))
        matrix[master, -1] = -matrix[master, :-1].sum()
    return names, matrix


def extrapolate(values):
    """Limit of a sequence whose differences shrink by a constant ratio."""
    f1, f2, f3 = values
    d1, d2 = f2 - f1, f3 - f2
    if d2 == 0 or d1 / d2 <= 1:
        return f3
    return f3 + d2 / (d1 / d2 - 1)


def main():
    results = []
    worst = 0.0
    for path in sys.argv[1:]:
        background, layers, boxes = read_structure(path)
        solutions = [solve(background, layers, boxes, h) for h in SPACINGS]
        names = solutions[0][0]
        matrices = np.array([m for _, m in solutions])
        structure = path.rsplit("/", 1)[-1].removesuffix(".txt")
        for i, master in enumerate(names):
            total = matrices[-1][i, i]
            for j, net in enumerate(names + ["ground"]):
                value = extrapolate(matrices[:, i, j])
                worst = max(worst, abs(value - matrices[-1][i, j]) / total)
                results.append(f"{structure} {master} {net} {value:.6g}")
    print("# fsv cross-section references from fv2d_golden.py, in fF/µm.")
    print(f"# Finite volumes at h = {', '.join(f'{h:g}' for h in SPACINGS)} µm,")
    print("# extrapolated to h = 0; the extrapolation moves no entry by more")
    print(f"# than {worst:.1e} of its row's total.")
    print("# <structure> <master> <net> <capacitance>; cap[master] is the total,")
    print("# other entries are minus the coupling (Maxwell matrix).")
    print("\n".join(results))


if __name__ == "__main__":
    main()
