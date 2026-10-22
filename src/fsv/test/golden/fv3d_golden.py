#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
"""3D reference capacitances for fsv golden structures.

Solves the 3D Laplace equation by finite volumes: a ground plane at z = 0,
planar layers, box conductors and conformal dielectrics, in the structure
format of fv2d_golden.py, whose conformal rules this applies in 3D (shells
grow on all four sides, line ends included, and from the union of a net's
boxes on a layer). Needs numpy, scipy and pyamg,
for example in a virtual environment:

  python3 -m venv venv && venv/bin/pip install numpy scipy pyamg
  venv/bin/python fv3d_golden.py crossing_conformal.txt > box_3d.txt

The grid is uniform with spacing h over the conductors and shells plus a
margin, and grows geometrically out to grounded far boundaries FAR µm away;
conductor faces, shell faces and layer interfaces lie on cell faces. Each
value is extrapolated from the SPACINGS grids assuming the error falls by a
constant ratio per halving. The header reports the largest extrapolation
step and the largest change when the far boundaries are moved twice as far
on the coarsest grid. Each solve reports its grid and time on stderr as it
finishes.
"""

import sys
import time

import numpy as np
import pyamg
import scipy.sparse as sp

from fv2d_golden import EPS0, extrapolate, read_structure

SPACINGS = (0.04, 0.02, 0.01)
MARGIN = 0.4
FAR = 20.0
GROWTH = 1.15


def grid_lines(breaks, lo, hi, h, far_lo, far_hi):
    """Lines spaced h on [lo, hi] plus every break, growing out to the far
    bounds."""
    lines = {round(v, 9) for v in np.arange(lo, hi + 1e-12, h)}
    lines |= {round(b, 9) for b in breaks if far_lo <= b <= far_hi}
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


def shells(boxes, conformals):
    """(name, eps, outer, inner, group, depth) of every conformal shell,
    innermost first per box, as in fv2d_golden.shells; outer and inner are
    (x0, y0, z0, x1, y1, z1)."""
    result = []
    groups = {}
    for net, b in boxes:
        inner = tuple(b)
        side = top = 0.0
        depth = 0
        for name, bottom, ztop, sw, tw, eps in conformals:
            if abs(b[2] - bottom) > 1e-9 or abs(b[5] - ztop) > 1e-9:
                continue
            side += sw
            top += tw
            depth += 1
            outer = (
                b[0] - side,
                b[1] - side,
                b[2],
                b[3] + side,
                b[4] + side,
                b[5] + top,
            )
            group = groups.setdefault((net, bottom, ztop), len(groups))
            result.append((name, eps, outer, inner, group, depth))
            inner = outer
    return result


def solve(background, layers, boxes, conformals, h, far=FAR):
    """The Maxwell matrix (fF), nets in first-seen order, with the ground
    column last."""
    names = list(dict.fromkeys(name for name, _ in boxes))
    shell_list = shells(boxes, conformals)
    rects = [tuple(b) for _, b in boxes] + [s[2] for s in shell_list]
    axes = []
    for k in range(3):
        breaks = [r[k] for r in rects] + [r[k + 3] for r in rects]
        if k == 2:
            lo = 0.0
            hi = max(breaks) + MARGIN
            breaks += [0.0] + [top for top, _ in layers]
            axes.append(grid_lines(breaks, lo, hi, h, 0.0, hi + far))
        else:
            lo, hi = min(breaks) - MARGIN, max(breaks) + MARGIN
            axes.append(grid_lines(breaks, lo, hi, h, lo - far, hi + far))
    centers = [0.5 * (a[1:] + a[:-1]) for a in axes]
    widths = [np.diff(a) for a in axes]
    shape = tuple(len(c) for c in centers)
    X, Y, Z = np.meshgrid(*centers, indexing="ij")

    def within(r):
        return (
            (X > r[0]) & (X < r[3]) & (Y > r[1]) & (Y < r[4]) & (Z > r[2]) & (Z < r[5])
        )

    zc = centers[2]
    eps_z = np.full(len(zc), background)
    for top, eps in reversed(layers):
        eps_z[zc < top] = eps
    eps = np.broadcast_to(eps_z[None, None, :], shape).copy()
    owner = -np.ones(shape, dtype=np.int32)
    for name, b in boxes:
        owner[within(b)] = names.index(name)
    free = owner < 0
    shell_of = np.full(shape, -1, dtype=np.int32)
    materials = list(dict.fromkeys(s[0] for s in shell_list))
    # The union rule of fv2d_golden.Grid: shallowest first, a cell keeps the
    # first shell that claims it when the next comes from the same group or
    # is the same conformal dielectric; another one from another group
    # overlaps.
    group_of = np.full(shape, -1, dtype=np.int32)
    for name, shell_eps, outer, _, group, _ in sorted(
        shell_list, key=lambda shell: shell[5]
    ):
        inside = within(outer) & free
        m = materials.index(name)
        claimed = inside & (shell_of >= 0)
        if (claimed & (shell_of != m) & (group_of != group)).any():
            raise ValueError(f"conformal dielectric {name} overlaps another")
        new = inside & (shell_of < 0)
        shell_of[new] = m
        group_of[new] = group
        eps[new] = shell_eps
    del X, Y, Z, shell_of, group_of

    index = -np.ones(shape, dtype=np.int64)
    index[free] = np.arange(int(free.sum()))
    n = int(free.sum())
    diag = np.zeros(n)
    rows, cols, vals = [], [], []
    wire_cell, wire_net, wire_g = [], [], []

    def along(values, k):
        """values (indexed along axis k) shaped to broadcast in 3D."""
        shape_k = [1, 1, 1]
        shape_k[k] = -1
        return values.reshape(shape_k)

    for k in range(3):
        # Area of the faces normal to axis k, and the cells' half-widths
        # along it.
        a, b = [j for j in range(3) if j != k]
        area = along(widths[a], a) * along(widths[b], b)
        half = 0.5 * widths[k]
        lo = tuple(slice(None, -1) if j == k else slice(None) for j in range(3))
        hi = tuple(slice(1, None) if j == k else slice(None) for j in range(3))
        r_lo = along(half[:-1], k) / eps[lo]
        r_hi = along(half[1:], k) / eps[hi]
        f_lo, f_hi = free[lo], free[hi]
        both = f_lo & f_hi
        g = np.broadcast_to(area, both.shape) / (r_lo + r_hi)
        gb = g[both]
        i_lo, i_hi = index[lo][both], index[hi][both]
        np.add.at(diag, i_lo, gb)
        np.add.at(diag, i_hi, gb)
        rows += [i_lo, i_hi]
        cols += [i_hi, i_lo]
        vals += [-gb, -gb]
        # A free cell next to a conductor cell: the conductor surface is the
        # shared face.
        for f_side, w_side, r_side, idx_side, own_side in (
            (f_lo, f_hi, r_lo, index[lo], owner[hi]),
            (f_hi, f_lo, r_hi, index[hi], owner[lo]),
        ):
            m = f_side & ~w_side
            gm = (np.broadcast_to(area, m.shape) / r_side)[m]
            np.add.at(diag, idx_side[m], gm)
            wire_cell.append(idx_side[m])
            wire_net.append(own_side[m])
            wire_g.append(gm)
        # Grounded boundaries at both ends of axis k (z = 0 is the ground
        # plane).
        for end in (0, -1):
            sl = tuple(end if j == k else slice(None) for j in range(3))
            m = free[sl]
            face_area = np.broadcast_to(area, free.shape)[sl]
            gm = (face_area / (0.5 * widths[k][end] / eps[sl]))[m]
            np.add.at(diag, index[sl][m], gm)
    rows = np.concatenate(rows + [np.arange(n)])
    cols = np.concatenate(cols + [np.arange(n)])
    vals = np.concatenate(vals + [diag])
    A = sp.csr_matrix((vals, (rows, cols)), shape=(n, n))
    wire_cell = np.concatenate(wire_cell)
    wire_net = np.concatenate(wire_net)
    wire_g = np.concatenate(wire_g)
    ml = pyamg.smoothed_aggregation_solver(A, symmetry="symmetric")

    matrix = np.zeros((len(names), len(names) + 1))
    for master in range(len(names)):
        on = wire_net == master
        rhs = np.zeros(n)
        np.add.at(rhs, wire_cell[on], wire_g[on])
        phi = ml.solve(rhs, tol=1e-11, accel="cg", maxiter=500)
        for net in range(len(names)):
            s = wire_net == net
            v = 1.0 if net == master else 0.0
            matrix[master, net] = EPS0 * np.sum(wire_g[s] * (v - phi[wire_cell[s]]))
        matrix[master, -1] = -matrix[master, :-1].sum()
    return names, matrix, n


def main():
    results = []
    worst = 0.0
    worst_far = 0.0
    for path in sys.argv[1:]:
        background, layers, boxes, conformals = read_structure(path)
        structure = path.rsplit("/", 1)[-1].removesuffix(".txt")

        def timed(h, far=FAR):
            start = time.time()
            result = solve(background, layers, boxes, conformals, h, far)
            print(
                f"{structure}: h = {h:g} µm, far {far:g} µm, {result[2]} cells, "
                f"{time.time() - start:.0f} s",
                file=sys.stderr,
                flush=True,
            )
            return result

        solutions = [timed(h) for h in SPACINGS]
        names = solutions[0][0]
        matrices = np.array([m for _, m, _ in solutions])
        _, far_matrix, _ = timed(SPACINGS[0], 2 * FAR)
        for i, master in enumerate(names):
            total = matrices[-1][i, i]
            for j, net in enumerate(names + ["ground"]):
                value = extrapolate(matrices[:, i, j])
                worst = max(worst, abs(value - matrices[-1][i, j]) / total)
                worst_far = max(
                    worst_far, abs(far_matrix[i, j] - matrices[0][i, j]) / total
                )
                results.append(f"{structure} {master} {net} {value:.6g}")
    print("# fsv 3D references from fv3d_golden.py, in fF.")
    print(f"# Finite volumes at h = {', '.join(f'{h:g}' for h in SPACINGS)} µm,")
    print("# extrapolated to h = 0; the extrapolation moves no entry by more")
    print(f"# than {worst:.1e} of its row's total, and doubling the distance to")
    print(f"# the grounded far boundaries ({FAR:g} µm) moves none by more than")
    print(f"# {worst_far:.1e}.")
    print("# <structure> <master> <net> <capacitance>; cap[master] is the total,")
    print("# other entries are minus the coupling (Maxwell matrix).")
    print("\n".join(results))


if __name__ == "__main__":
    main()
