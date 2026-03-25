#!/usr/bin/env python3
"""Plot scan chain cell positions and connections before/after optimization.

Usage:
  python3 plot_scan_chain.py results/scan_chain_before.csv [results/scan_chain_after.csv]

Reads CSV files with columns: name,x,y
Rows are in chain order (first row = first cell in chain).
"""

import csv
import sys
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches


def read_chain(path):
    names, xs, ys = [], [], []
    with open(path) as f:
        reader = csv.DictReader(f)
        for row in reader:
            names.append(row["name"])
            xs.append(int(row["x"]))
            ys.append(int(row["y"]))
    return names, xs, ys


def manhattan_wl(xs, ys):
    total = 0
    for i in range(len(xs) - 1):
        total += abs(xs[i + 1] - xs[i]) + abs(ys[i + 1] - ys[i])
    return total


def plot_chain(ax, names, xs, ys, title, color):
    wl = manhattan_wl(xs, ys)
    ax.set_title(f"{title}\nWirelength: {wl:,}")

    # Draw connections
    for i in range(len(xs) - 1):
        ax.annotate(
            "",
            xy=(xs[i + 1], ys[i + 1]),
            xytext=(xs[i], ys[i]),
            arrowprops=dict(arrowstyle="->", color=color, lw=1.5, alpha=0.6),
        )

    # Draw cells
    ax.scatter(xs, ys, s=80, c=color, zorder=5, edgecolors="black", linewidths=0.5)

    # Label cells with chain index
    for i, name in enumerate(names):
        ax.annotate(
            str(i),
            (xs[i], ys[i]),
            textcoords="offset points",
            xytext=(5, 5),
            fontsize=7,
            color="black",
        )

    ax.set_xlabel("X")
    ax.set_ylabel("Y")
    ax.set_aspect("equal")
    ax.grid(True, alpha=0.3)


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)

    before_path = sys.argv[1]
    after_path = sys.argv[2] if len(sys.argv) > 2 else None

    if after_path:
        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))
        names_b, xs_b, ys_b = read_chain(before_path)
        names_a, xs_a, ys_a = read_chain(after_path)

        plot_chain(ax1, names_b, xs_b, ys_b, "Before scan_opt", "tab:red")
        plot_chain(ax2, names_a, xs_a, ys_a, "After scan_opt", "tab:blue")

        wl_b = manhattan_wl(xs_b, ys_b)
        wl_a = manhattan_wl(xs_a, ys_a)
        if wl_b > 0:
            pct = 100.0 * (wl_b - wl_a) / wl_b
            fig.suptitle(
                f"Scan Chain Optimization ({pct:+.1f}% wirelength change)",
                fontsize=13,
                fontweight="bold",
            )
    else:
        fig, ax = plt.subplots(figsize=(7, 6))
        names, xs, ys = read_chain(before_path)
        plot_chain(ax, names, xs, ys, "Scan Chain", "tab:blue")

    plt.tight_layout()
    out_path = "scan_chain_comparison.png"
    plt.savefig(out_path, dpi=150)
    print(f"Saved to {out_path}")


if __name__ == "__main__":
    main()
