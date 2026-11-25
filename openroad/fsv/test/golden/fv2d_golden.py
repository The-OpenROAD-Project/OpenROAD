#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
"""Per-unit-length reference capacitances for fsv cross-section structures.

Solves the 2D Laplace equation in the x-z plane by finite volumes: a ground
plane at z = 0, planar layers, and wires infinitely long along y (the box y
extents are ignored). Needs numpy and scipy:

  python3 fv2d_golden.py etch_stop_pair.txt three_wires.txt > cross_section.txt

To see a structure and its grid instead (needs matplotlib):

  python3 fv2d_golden.py --plot plots --h 0.01 nested_conformal.txt

writes plots/nested_conformal.png: the uniform region around the wires with
every cell, and the whole domain out to the grounded far boundaries.

The grid is uniform with spacing h around the wires and their shells, from
the ground plane up, and grows geometrically out to grounded far boundaries
FAR µm away; wire faces, shell faces and layer interfaces lie on cell
faces. Each value is extrapolated from h = 0.01, 0.005 and
0.0025 µm, assuming the error falls by a constant ratio per halving (the
corners make the order fractional, about 1.3). The header reports the largest
change from the finest grid to the extrapolated value.

Conformal dielectrics follow the rules in the fsv plan, applied here
independently of fsv: a line "conformal <name> <bottom> <top> <SW_T> <TW_T>
<eps>" wraps every box whose z extent is [bottom, top]; lines for the same
extent nest outward in order. Conformal k of a box is the box grown by the
sum of the first k SW_T on its sides and the first k TW_T on top, with its
bottom kept. The boxes of one net on one layer grow their shells from their
union: a point takes the shallowest conformal any of them puts there.
Conductors cut into conformal dielectrics, conformal dielectrics replace the
planar layers they cross, shells of the same conformal dielectric merge, and
otherwise shells of different ones must not overlap.
"""

import argparse
import os
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
    """(background, layers, boxes, conformals) of a structure file."""
    layers, boxes, conformals, background = [], [], [], None
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
            elif words[0] == "conformal":
                conformals.append((words[1], *[float(v) for v in words[2:7]]))
            else:
                raise ValueError(f"{path}: unknown line {line.strip()}")
    return background, layers, boxes, conformals


def shells(boxes, conformals):
    """(name, eps, outer, inner, group, depth) of every conformal shell,
    innermost first per box. outer and inner are (x0, z0, x1, z1): the box
    grown by this conformal and those inside it, and by those inside it only.
    The boxes of one net on one layer form a group (a number), whose shells
    grow from the union of its boxes; depth is 1 for the innermost
    conformal."""
    result = []
    groups = {}
    for net, b in boxes:
        inner = (b[0], b[2], b[3], b[5])
        side = top = 0.0
        depth = 0
        for name, bottom, ztop, sw, tw, eps in conformals:
            if abs(b[2] - bottom) > 1e-9 or abs(b[5] - ztop) > 1e-9:
                continue
            side += sw
            top += tw
            depth += 1
            outer = (b[0] - side, b[2], b[3] + side, b[5] + top)
            group = groups.setdefault((net, bottom, ztop), len(groups))
            result.append((name, eps, outer, inner, group, depth))
            inner = outer
    return result


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


class Grid:
    """A structure discretized at spacing h: the grid lines X and Z, and per
    cell its permittivity, the net it belongs to (-1 outside conductors) and
    its conformal dielectric ("" outside shells)."""

    def __init__(self, background, layers, boxes, h, conformals=()):
        self.names = list(dict.fromkeys(name for name, _ in boxes))
        self.shells = shells(boxes, conformals)
        x_breaks = [v for _, b in boxes for v in (b[0], b[3])]
        x_breaks += [v for s in self.shells for v in (s[2][0], s[2][2])]
        z_breaks = [v for _, b in boxes for v in (b[2], b[5])]
        z_breaks += [v for s in self.shells for v in (s[2][1], s[2][3])]
        # The uniform grid covers the wires and shells; layer tops above it
        # are grid lines in the growing cells.
        z_hi = max(z_breaks) + MARGIN
        z_breaks += [top for top, _ in layers]
        x_lo, x_hi = min(x_breaks) - MARGIN, max(x_breaks) + MARGIN
        # The uniform region, (x0, z0, x1, z1).
        self.near = (x_lo, 0.0, x_hi, z_hi)
        self.X = grid_lines(x_breaks, x_lo, x_hi, h, x_lo - FAR, x_hi + FAR)
        self.Z = grid_lines(z_breaks + [0.0], 0.0, z_hi, h, 0.0, z_hi + FAR)
        xc = 0.5 * (self.X[1:] + self.X[:-1])
        zc = 0.5 * (self.Z[1:] + self.Z[:-1])
        shape = (len(xc), len(zc))

        def eps_at(z):
            for top, eps in layers:
                if z < top:
                    return eps
            return background

        self.eps = np.repeat(
            np.array([eps_at(z) for z in zc])[None, :], shape[0], axis=0
        )
        self.owner = -np.ones(shape, dtype=int)
        for name, b in boxes:
            inside = (
                (xc[:, None] > b[0])
                & (xc[:, None] < b[3])
                & (zc[None, :] > b[2])
                & (zc[None, :] < b[5])
            )
            self.owner[inside] = self.names.index(name)
        free = self.owner < 0
        # Conformal dielectrics replace the planar layers; conductors cut
        # into them.
        self.shell_of = np.full(shape, "", dtype=object)

        def within(r):
            return (
                (xc[:, None] > r[0])
                & (xc[:, None] < r[2])
                & (zc[None, :] > r[1])
                & (zc[None, :] < r[3])
            )

        # Shallowest first: a cell keeps the first shell that claims it when
        # the next comes from the same group (the union rule) or is the same
        # conformal dielectric; another one from another group overlaps.
        group_of = np.full(shape, -1, dtype=int)
        for name, shell_eps, outer, _, group, _ in sorted(
            self.shells, key=lambda shell: shell[5]
        ):
            inside = within(outer) & free
            claimed = inside & (self.shell_of != "")
            clash = claimed & (self.shell_of != name) & (group_of != group)
            if clash.any():
                other = self.shell_of[clash][0]
                raise ValueError(f"conformal dielectrics {other} and {name} overlap")
            new = inside & (self.shell_of == "")
            self.shell_of[new] = name
            group_of[new] = group
            self.eps[new] = shell_eps


def solve(background, layers, boxes, h, conformals=()):
    """The Maxwell matrix per unit length (fF/µm), nets in first-seen
    order, with the ground column last."""
    grid = Grid(background, layers, boxes, h, conformals)
    names = grid.names
    eps, owner = grid.eps, grid.owner
    dx, dz = np.diff(grid.X), np.diff(grid.Z)
    nx, nz = eps.shape
    free = owner < 0
    index = -np.ones((nx, nz), dtype=int)
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

    I, J = np.meshgrid(np.arange(nx), np.arange(nz), indexing="ij")
    # Neighbors along x, then along z.
    a = (I[:-1, :], J[:-1, :])
    b = (I[1:, :], J[1:, :])
    couple(a, b, dz[J[:-1, :]], dx[I[:-1, :]] / 2, dx[I[1:, :]] / 2)
    a = (I[:, :-1], J[:, :-1])
    b = (I[:, 1:], J[:, 1:])
    couple(a, b, dx[I[:, :-1]], dz[J[:, :-1]] / 2, dz[J[:, 1:]] / 2)
    # Grounded boundaries: the plane at z = 0 and the far edges.
    for cells, area, half in (
        ((I[:, 0], J[:, 0]), dx, dz[0] / 2),
        ((I[:, -1], J[:, -1]), dx, dz[-1] / 2),
        ((I[0, :], J[0, :]), dz, dx[0] / 2),
        ((I[-1, :], J[-1, :]), dz, dx[-1] / 2),
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


def plot(path, out_dir, h):
    """Writes <out_dir>/<structure>.png: the permittivity of every cell, the
    conductors, the conformal shells, the planar interfaces and the grid at
    spacing h, near the wires and over the whole domain."""
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.colors as mcolors
    import matplotlib.patches as mpatches
    import matplotlib.pyplot as plt

    background, layers, boxes, conformals = read_structure(path)
    grid = Grid(background, layers, boxes, h, conformals)
    structure = os.path.basename(path).removesuffix(".txt")
    ink, muted, copper = "#17202a", "#586472", "#b5652b"
    eps_values = grid.eps[grid.owner < 0]
    norm = mcolors.Normalize(vmin=min(1.0, eps_values.min()), vmax=eps_values.max())
    shell_styles = ["-", "--", ":", "-."]
    materials = list(dict.fromkeys(name for name, *_ in grid.shells))

    fig, axes = plt.subplots(
        1,
        2,
        figsize=(14, 6),
        gridspec_kw={"width_ratios": [1.3, 1]},
        layout="constrained",
    )
    for ax, near in zip(axes, (True, False)):
        mesh = ax.pcolormesh(
            grid.X,
            grid.Z,
            np.ma.masked_where(grid.owner >= 0, grid.eps).T,
            cmap="Blues",
            norm=norm,
            edgecolors=(0.55, 0.6, 0.65, 0.5),
            linewidth=0.15 if near else 0.05,
        )
        ax.pcolormesh(
            grid.X,
            grid.Z,
            np.ma.masked_where(grid.owner < 0, np.zeros(grid.owner.shape)).T,
            cmap=mcolors.ListedColormap([copper]),
        )
        ax.axhline(0, color=ink, linewidth=3)
        if near:
            for name, b in boxes:
                ax.text(
                    0.5 * (b[0] + b[3]),
                    0.5 * (b[2] + b[5]),
                    name,
                    color="white",
                    ha="center",
                    va="center",
                    fontsize=9,
                    family="monospace",
                )
            for name, shell_eps, outer, *_ in grid.shells:
                ax.add_patch(
                    mpatches.Rectangle(
                        (outer[0], outer[1]),
                        outer[2] - outer[0],
                        outer[3] - outer[1],
                        fill=False,
                        edgecolor=ink,
                        linewidth=1.0,
                        linestyle=shell_styles[materials.index(name) % 4],
                    )
                )
            x0, z0, x1, z1 = grid.near
            bottom = 0.0
            for top, eps in layers:
                if top <= z1:
                    ax.axhline(top, color=muted, linewidth=0.8, linestyle=(0, (1, 2)))
                mid = 0.5 * (bottom + min(top, z1))
                if bottom < z1:
                    ax.text(
                        x1, mid, f" ε {eps:g}", color=muted, va="center", fontsize=8
                    )
                bottom = top
            if bottom < z1:
                ax.text(
                    x1,
                    0.5 * (bottom + z1),
                    f" ε {background:g}",
                    color=muted,
                    va="center",
                    fontsize=8,
                )
            ax.set_xlim(x0, x1)
            ax.set_ylim(z0, z1)
            ax.set_title("Uniform region: every cell", color=ink, fontsize=11)
            handles = [
                mpatches.Patch(color=copper, label="conductor"),
                *[
                    plt.Line2D(
                        [],
                        [],
                        color=ink,
                        linestyle=shell_styles[i % 4],
                        label=f"{m} (ε {next(e for n, e, *_ in grid.shells if n == m):g})",
                    )
                    for i, m in enumerate(materials)
                ],
            ]
            ax.legend(handles=handles, loc="upper left", fontsize=8, framealpha=0.9)
        else:
            x0, z0, x1, z1 = grid.near
            ax.add_patch(
                mpatches.Rectangle(
                    (x0, z0),
                    x1 - x0,
                    z1 - z0,
                    fill=False,
                    edgecolor="#b0307a",
                    linewidth=1.2,
                )
            )
            ax.set_xlim(grid.X[0], grid.X[-1])
            ax.set_ylim(grid.Z[0], grid.Z[-1])
            ax.set_title(
                "Whole domain, out to the grounded far boundaries",
                color=ink,
                fontsize=11,
            )
        ax.set_aspect("equal")
        ax.set_xlabel("x (µm)", color=muted)
        ax.set_ylabel("z (µm)", color=muted)
        ax.tick_params(colors=muted, labelsize=8)
    bar = fig.colorbar(mesh, ax=axes, shrink=0.8)
    bar.set_label("relative permittivity", color=muted)
    bar.ax.tick_params(colors=muted, labelsize=8)
    cells = grid.eps.size
    fig.suptitle(
        f"{structure}: h = {h:g} µm, {cells} cells "
        f"({len(grid.X) - 1} × {len(grid.Z) - 1})",
        color=ink,
        fontsize=13,
    )
    os.makedirs(out_dir, exist_ok=True)
    out = os.path.join(out_dir, f"{structure}.png")
    fig.savefig(out, dpi=200)
    plt.close(fig)
    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("structures", nargs="+", help="structure files")
    parser.add_argument(
        "--plot",
        metavar="DIR",
        help="draw each structure and its grid into DIR instead of solving",
    )
    parser.add_argument(
        "--h",
        type=float,
        default=SPACINGS[0],
        help=f"grid spacing for --plot, µm (default {SPACINGS[0]:g})",
    )
    args = parser.parse_args()
    if args.plot:
        for path in args.structures:
            print(plot(path, args.plot, args.h))
        return

    results = []
    worst = 0.0
    for path in args.structures:
        background, layers, boxes, conformals = read_structure(path)
        solutions = [solve(background, layers, boxes, h, conformals) for h in SPACINGS]
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
