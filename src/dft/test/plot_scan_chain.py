#!/usr/bin/env python3
"""Plot scan chain cell order before and after optimization."""
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

def chain_wirelength(cells):
    wl = 0
    for i in range(len(cells) - 1):
        dx = abs(cells[i+1][1] - cells[i][1])
        dy = abs(cells[i+1][2] - cells[i][2])
        wl += dx + dy
    return wl

def plot_chain(ax, cells, title, color):
    xs = [c[1] for c in cells]
    ys = [c[2] for c in cells]

    for i in range(len(cells) - 1):
        ax.annotate('', xy=(xs[i+1], ys[i+1]), xytext=(xs[i], ys[i]),
                     arrowprops=dict(arrowstyle='->', color=color, lw=1.5, alpha=0.7))

    ax.scatter(xs, ys, c='navy', s=80, zorder=5)

    for i, (name, x, y) in enumerate(cells):
        short = name.replace('_clk1_rising', '')
        ax.annotate(f'{i}: {short}', (x, y), textcoords="offset points",
                     xytext=(5, 8), fontsize=7, fontweight='bold')

    wl = chain_wirelength(cells)
    ax.set_title(f'{title}\nWirelength: {wl:,}', fontsize=11)
    ax.set_xlabel('X')
    ax.set_ylabel('Y')
    ax.grid(True, alpha=0.3)
    ax.set_aspect('equal')

# Positions of cells (from the post-DP placement)
pos = {
    'ff1':  (1000,  1000),
    'ff2':  (20000, 2000),
    'ff3':  (2000,  3000),
    'ff4':  (19000, 4000),
    'ff5':  (3000,  5000),
    'ff6':  (18000, 6000),
    'ff7':  (4000,  7000),
    'ff8':  (17000, 8000),
    'ff9':  (5000,  9000),
    'ff10': (16000, 10000),
}

# Before: scan_in_0 -> ff9 -> ff8 -> ff7 -> ff6 -> ff5 -> ff4 -> ff3 -> ff2 -> ff1 -> ff10
before_order = ['ff9', 'ff8', 'ff7', 'ff6', 'ff5', 'ff4', 'ff3', 'ff2', 'ff1', 'ff10']
before = [(n, pos[n][0], pos[n][1]) for n in before_order]

# After: scan_in_0 -> ff1 -> ff3 -> ff5 -> ff7 -> ff9 -> ff10 -> ff8 -> ff6 -> ff4 -> ff2
after_order = ['ff1', 'ff3', 'ff5', 'ff7', 'ff9', 'ff10', 'ff8', 'ff6', 'ff4', 'ff2']
after = [(n, pos[n][0], pos[n][1]) for n in after_order]

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 7))
plot_chain(ax1, before, 'Before scan_opt (architect order)', 'red')
plot_chain(ax2, after, 'After scan_opt (optimized order)', 'green')

wl_before = chain_wirelength(before)
wl_after = chain_wirelength(after)
reduction = (1 - wl_after / wl_before) * 100

fig.suptitle(f'Scan Chain Wirelength Optimization\n'
             f'Reduction: {wl_before:,} -> {wl_after:,} ({reduction:.1f}% improvement)',
             fontsize=13, fontweight='bold')
plt.tight_layout()
plt.savefig('/home/qhmoso/work/OpenROAD_Work/OpenROAD/src/dft/test/scan_chain_comparison.png', dpi=150)
print(f"Before wirelength: {wl_before:,}")
print(f"After wirelength:  {wl_after:,}")
print(f"Reduction: {reduction:.1f}%")
print("Saved to src/dft/test/scan_chain_comparison.png")
