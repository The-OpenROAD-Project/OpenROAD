#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors

"""Build dependency analysis and compile-time estimation for OpenROAD.

Extracts C++ include and Bazel dependency graphs, analyzes git history,
and plots estimated rebuild cost per commit. Includes "pruned header"
scenarios showing potential gains from reducing header coupling.

Usage:
    bazelisk run //tools/stats:compile_time

Outputs:
    tools/stats/cpp_deps.yaml   - C++ include dependency graph
    tools/stats/bazel_deps.yaml - Bazel target dependency graph
    tools/stats/compile_time.png - Rebuild cost plot
"""

import argparse
import collections
import datetime
import os
import re
import subprocess
import sys
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
import yaml


def find_project_root():
    """Find the OpenROAD project root (directory containing MODULE.bazel)."""
    # When run via bazel run, BUILD_WORKSPACE_DIRECTORY is set
    ws = os.environ.get("BUILD_WORKSPACE_DIRECTORY")
    if ws:
        return Path(ws)
    # Fallback: walk up from script location
    p = Path(__file__).resolve().parent
    while p != p.parent:
        if (p / "MODULE.bazel").exists():
            return p
        p = p.parent
    sys.exit("Error: cannot find project root (no MODULE.bazel found)")


# ---------------------------------------------------------------------------
# C++ Include Graph
# ---------------------------------------------------------------------------

SOURCE_EXTENSIONS = {".cpp", ".cc", ".h", ".hpp", ".hh"}
INCLUDE_RE = re.compile(r'^\s*#\s*include\s+"([^"]+)"', re.MULTILINE)


def collect_source_files(root):
    """Collect all C++ source/header files under src/ and include/."""
    files = []
    for search_dir in ["src", "include"]:
        base = root / search_dir
        if not base.exists():
            continue
        for path in base.rglob("*"):
            if path.suffix in SOURCE_EXTENSIONS and path.is_file():
                files.append(path.relative_to(root))
    return files


def build_include_lookup(root, source_files):
    """Build a dict mapping possible include strings to actual file paths.

    For example: "drt/TritonRoute.h" -> "src/drt/include/drt/TritonRoute.h"
    """
    lookup = {}

    for f in source_files:
        parts = f.parts
        # Direct path from project root
        lookup[str(f)] = f

        # For files under src/<module>/include/<module>/Foo.h,
        # the include string is "<module>/Foo.h"
        if len(parts) >= 4 and parts[0] == "src" and parts[2] == "include":
            include_str = str(Path(*parts[3:]))
            # Don't overwrite if already exists (first match wins)
            if include_str not in lookup:
                lookup[include_str] = f

        # For files under src/<module>/src/subdir/Foo.h,
        # the include string might be "subdir/Foo.h" or just "Foo.h"
        if len(parts) >= 4 and parts[0] == "src" and parts[2] == "src":
            # Relative from the src/ subdir: "subdir/Foo.h"
            include_str = str(Path(*parts[3:]))
            if include_str not in lookup:
                lookup[include_str] = f
            # Also just filename
            if parts[-1] not in lookup:
                lookup[str(parts[-1])] = f

        # For files under include/ord/Foo.h, include string is "ord/Foo.h"
        if len(parts) >= 2 and parts[0] == "include":
            include_str = str(Path(*parts[1:]))
            if include_str not in lookup:
                lookup[include_str] = f

    return lookup


def build_cpp_include_graph(root):
    """Parse all source files and build the C++ include dependency graph.

    Returns:
        graph: dict mapping file path (str) -> list of included file paths (str)
        source_files: list of all source file paths (relative to root)
    """
    source_files = collect_source_files(root)
    lookup = build_include_lookup(root, source_files)

    graph = {}
    unresolved = collections.Counter()

    for f in source_files:
        fstr = str(f)
        full_path = root / f
        try:
            content = full_path.read_text(errors="replace")
        except OSError:
            continue

        includes = INCLUDE_RE.findall(content)
        resolved = []
        file_dir = f.parent

        for inc in includes:
            # Try resolution in order of specificity
            target = None

            # 1. Relative to the including file's directory
            candidate = file_dir / inc
            candidate_str = str(candidate)
            if candidate_str in lookup:
                target = str(lookup[candidate_str])
            elif (root / candidate).is_file() and candidate in [
                sf for sf in source_files
            ]:
                target = candidate_str

            # 2. Direct lookup (covers module/Foo.h patterns)
            if target is None and inc in lookup:
                target = str(lookup[inc])

            # 3. Try prefixing with src/<same_module>/src/
            if target is None and len(f.parts) >= 2 and f.parts[0] == "src":
                module = f.parts[1]
                candidate = Path("src") / module / "src" / inc
                candidate_str = str(candidate)
                if candidate_str in lookup:
                    target = str(lookup[candidate_str])

            if target is not None:
                resolved.append(target)
            else:
                unresolved[inc] += 1

        graph[fstr] = resolved

    if unresolved:
        print(f"  {len(unresolved)} unique unresolved includes (external deps)")
        top5 = unresolved.most_common(5)
        for inc, count in top5:
            print(f"    {inc} ({count} files)")

    return graph, source_files


def build_reverse_graph(graph):
    """Build reverse dependency graph: for each file, who includes it."""
    reverse = collections.defaultdict(set)
    for src, deps in graph.items():
        for dep in deps:
            reverse[dep].add(src)
    return reverse


# ---------------------------------------------------------------------------
# Bazel Dependency Graph
# ---------------------------------------------------------------------------


def build_bazel_graph(root):
    """Extract Bazel cc_library dependency graph using bazel query."""
    print("Extracting Bazel dependency graph...")
    try:
        result = subprocess.run(
            [
                "bazel",
                "query",
                'kind("cc_library", //src/...)',
                "--output=graph",
                "--noimplicit_deps",
            ],
            capture_output=True,
            text=True,
            cwd=root,
            timeout=120,
        )
    except FileNotFoundError:
        # Try bazelisk
        result = subprocess.run(
            [
                "bazelisk",
                "query",
                'kind("cc_library", //src/...)',
                "--output=graph",
                "--noimplicit_deps",
            ],
            capture_output=True,
            text=True,
            cwd=root,
            timeout=120,
        )

    if result.returncode != 0:
        print(f"Warning: bazel query failed: {result.stderr[:500]}")
        return {}

    # Parse DOT format: "//src/X" -> "//src/Y"
    edge_re = re.compile(r'"(//src/[^"]+)"\s*->\s*"(//src/[^"]+)"')
    graph = collections.defaultdict(list)
    all_targets = set()

    for line in result.stdout.splitlines():
        m = edge_re.search(line)
        if m:
            src, dst = m.group(1), m.group(2)
            graph[src].append(dst)
            all_targets.add(src)
            all_targets.add(dst)

    # Ensure all targets appear as keys
    for t in all_targets:
        if t not in graph:
            graph[t] = []

    print(f"  Found {len(all_targets)} Bazel targets, {sum(len(v) for v in graph.values())} edges")
    return dict(graph)


# ---------------------------------------------------------------------------
# Module mapping and LOC counting
# ---------------------------------------------------------------------------


def file_to_module(filepath):
    """Map a file path to its functional unit (module name)."""
    parts = Path(filepath).parts
    if len(parts) >= 2 and parts[0] == "src":
        if parts[1] == "cmake":
            return None
        return parts[1]
    if parts[0] == "include":
        return "ord"
    if parts[0] == "src":
        return "ord"
    return None


def target_to_module(target):
    """Map a Bazel target label to its module name."""
    # //src/drt:foo -> drt
    m = re.match(r"//src/([^/:]+)", target)
    return m.group(1) if m else None


def count_loc(root, source_files):
    """Count lines of code per source file."""
    loc = {}
    for f in source_files:
        try:
            loc[str(f)] = sum(1 for _ in open(root / f, "rb"))
        except OSError:
            loc[str(f)] = 0
    return loc


# ---------------------------------------------------------------------------
# Worst header analysis
# ---------------------------------------------------------------------------


def compute_header_rebuild_costs(reverse_graph, loc, source_files):
    """For each .h file, compute total LOC of .cpp/.cc files that
    transitively depend on it.

    Returns dict: header_path -> total_rebuild_loc
    """
    headers = [
        str(f)
        for f in source_files
        if f.suffix in {".h", ".hpp", ".hh"}
    ]
    cpp_files = {
        str(f)
        for f in source_files
        if f.suffix in {".cpp", ".cc"}
    }

    costs = {}
    for h in headers:
        # BFS through reverse graph to find all transitive dependents
        visited = set()
        queue = collections.deque([h])
        total_loc = 0
        while queue:
            node = queue.popleft()
            if node in visited:
                continue
            visited.add(node)
            if node in cpp_files:
                total_loc += loc.get(node, 0)
            for dep in reverse_graph.get(node, []):
                if dep not in visited:
                    queue.append(dep)
        costs[h] = total_loc

    return costs


def rank_worst_headers(header_costs, top_n=10):
    """Return top-N headers sorted by rebuild cost."""
    return sorted(header_costs.items(), key=lambda x: -x[1])[:top_n]


# ---------------------------------------------------------------------------
# Git history analysis
# ---------------------------------------------------------------------------


def get_commits(root, months=3):
    """Get list of (hash, timestamp) for non-merge commits in last N months."""
    since = (
        datetime.datetime.now() - datetime.timedelta(days=months * 30)
    ).strftime("%Y-%m-%d")
    result = subprocess.run(
        ["git", "log", f"--since={since}", "--format=%H %ct", "--no-merges"],
        capture_output=True,
        text=True,
        cwd=root,
    )
    commits = []
    for line in result.stdout.strip().splitlines():
        if not line.strip():
            continue
        parts = line.split()
        commits.append((parts[0], int(parts[1])))
    return commits


def get_changed_files(root, commit_hash):
    """Get list of changed files for a commit."""
    result = subprocess.run(
        ["git", "diff-tree", "--no-commit-id", "-r", "--name-only", commit_hash],
        capture_output=True,
        text=True,
        cwd=root,
    )
    files = []
    for line in result.stdout.strip().splitlines():
        line = line.strip()
        if line and Path(line).suffix in SOURCE_EXTENSIONS:
            files.append(line)
    return files


def compute_rebuild_loc(changed_files, reverse_graph, loc, pruned_headers=None):
    """Compute total LOC that must be recompiled for a set of changed files.

    Args:
        changed_files: list of changed file paths
        reverse_graph: dict of file -> set of files that include it
        loc: dict of file -> line count
        pruned_headers: set of header paths to exclude from propagation

    Returns:
        Total lines of .cpp/.cc code to recompile
    """
    pruned = pruned_headers or set()
    affected_cpp = set()

    for f in changed_files:
        if Path(f).suffix in {".cpp", ".cc"}:
            affected_cpp.add(f)
        # BFS through reverse graph for headers
        if Path(f).suffix in {".h", ".hpp", ".hh"}:
            if f in pruned:
                continue
            visited = set()
            queue = collections.deque([f])
            while queue:
                node = queue.popleft()
                if node in visited:
                    continue
                visited.add(node)
                if Path(node).suffix in {".cpp", ".cc"}:
                    affected_cpp.add(node)
                for dep in reverse_graph.get(node, []):
                    if dep not in visited and dep not in pruned:
                        queue.append(dep)

    return sum(loc.get(f, 0) for f in affected_cpp)


def compute_bazel_affected_modules(changed_files, bazel_graph):
    """Compute set of Bazel functional units affected by changed files.

    Maps changed files to likely targets, then walks reverse deps.
    """
    if not bazel_graph:
        return set()

    # Build reverse Bazel graph
    bazel_reverse = collections.defaultdict(set)
    for src, deps in bazel_graph.items():
        for dep in deps:
            bazel_reverse[dep].add(src)

    # Map changed files to likely Bazel targets
    affected_targets = set()
    for f in changed_files:
        module = file_to_module(f)
        if module is None:
            continue
        # Find matching targets for this module
        for target in bazel_graph:
            if target_to_module(target) == module:
                affected_targets.add(target)

    # BFS through Bazel reverse graph
    visited = set()
    queue = collections.deque(affected_targets)
    while queue:
        t = queue.popleft()
        if t in visited:
            continue
        visited.add(t)
        for dep in bazel_reverse.get(t, []):
            if dep not in visited:
                queue.append(dep)

    # Map to modules
    modules = set()
    for t in visited:
        m = target_to_module(t)
        if m:
            modules.add(m)
    return modules


# ---------------------------------------------------------------------------
# Plotting
# ---------------------------------------------------------------------------


def rolling_average(values, window=20):
    """Compute rolling average with given window size."""
    result = []
    for i in range(len(values)):
        start = max(0, i - window + 1)
        result.append(sum(values[start : i + 1]) / (i - start + 1))
    return result


def generate_plot(dates, series_dict, output_path):
    """Generate the compile-time estimation plot.

    Args:
        dates: list of datetime objects
        series_dict: dict of series_name -> list of values
        output_path: Path to save the PNG
    """
    fig, ax1 = plt.subplots(figsize=(16, 9))

    colors = [
        "#2196F3",  # Full C++ deps - blue
        "#4CAF50",  # Pruned 1 - green
        "#8BC34A",  # Pruned 2
        "#CDDC39",  # Pruned 3
        "#FFC107",  # Pruned 4
        "#FF9800",  # Pruned 5
    ]
    bazel_color = "#E91E63"  # Bazel - pink

    color_idx = 0
    for name, values in series_dict.items():
        if name == "Bazel functional units":
            continue
        smoothed = rolling_average(values)
        ax1.plot(
            dates,
            [v / 1000 for v in smoothed],  # Convert to KLOC
            label=name,
            color=colors[color_idx % len(colors)],
            linewidth=1.5 if color_idx == 0 else 1.0,
            alpha=1.0 if color_idx == 0 else 0.7,
        )
        color_idx += 1

    ax1.set_xlabel("Commit Date")
    ax1.set_ylabel("Estimated Rebuild Size (KLOC)")
    ax1.xaxis.set_major_formatter(mdates.DateFormatter("%Y-%m-%d"))
    ax1.xaxis.set_major_locator(mdates.WeekdayLocator(interval=2))
    fig.autofmt_xdate()

    # Secondary y-axis for Bazel functional units
    if "Bazel functional units" in series_dict:
        ax2 = ax1.twinx()
        smoothed = rolling_average(series_dict["Bazel functional units"])
        ax2.plot(
            dates,
            smoothed,
            label="Bazel functional units",
            color=bazel_color,
            linewidth=1.5,
            linestyle="--",
        )
        ax2.set_ylabel("Bazel Functional Units Affected")
        # Combine legends
        lines1, labels1 = ax1.get_legend_handles_labels()
        lines2, labels2 = ax2.get_legend_handles_labels()
        ax1.legend(lines1 + lines2, labels1 + labels2, loc="upper left", fontsize=8)
    else:
        ax1.legend(loc="upper left", fontsize=8)

    ax1.set_title("OpenROAD Estimated Rebuild Cost per Commit (3-month history)")
    ax1.grid(True, alpha=0.3)
    fig.tight_layout()
    fig.savefig(output_path, dpi=150)
    plt.close(fig)
    print(f"Plot saved to {output_path}")


def generate_dep_tree_plot(cpp_graph, reverse_graph, loc, source_files, output_path):
    """Render the include dependency tree as a visual graph.

    Shows the critical include chain from Logger.h through odb/db.h
    to consumer modules, with node size proportional to rebuild cost.
    """
    total_cpp_loc = sum(
        loc.get(str(f), 0) for f in source_files if f.suffix in {".cpp", ".cc"}
    )
    cpp_files = {str(f) for f in source_files if f.suffix in {".cpp", ".cc"}}

    def rebuild_cost(header):
        visited = set()
        queue = collections.deque([header])
        total = 0
        while queue:
            node = queue.popleft()
            if node in visited:
                continue
            visited.add(node)
            if node in cpp_files:
                total += loc.get(node, 0)
            for dep in reverse_graph.get(node, set()):
                if dep not in visited:
                    queue.append(dep)
        return total

    def ext_cpp_count(header):
        visited = set()
        queue = collections.deque([header])
        count = 0
        while queue:
            node = queue.popleft()
            if node in visited:
                continue
            visited.add(node)
            if node in cpp_files and not node.startswith("src/odb/"):
                count += 1
            for dep in reverse_graph.get(node, set()):
                if dep not in visited:
                    queue.append(dep)
        return count

    # Define tree structure: (label, path, children, tier_level)
    # Tier 0: Logger.h
    # Tier 1: odb headers inside db.h
    # Tier 2: dbSta bridge
    # Tier 3: module public headers

    # Consumer modules (tier 2 and 3)
    consumers = [
        ("db_sta/dbSta.hh", "src/dbSta/include/db_sta/dbSta.hh"),
        ("db_sta/dbNetwork.hh", "src/dbSta/include/db_sta/dbNetwork.hh"),
        ("stt/SteinerTreeBuilder.h", "src/stt/include/stt/SteinerTreeBuilder.h"),
        ("grt/GlobalRouter.h", "src/grt/include/grt/GlobalRouter.h"),
        ("dpl/Opendp.h", "src/dpl/include/dpl/Opendp.h"),
        ("gui/gui.h", "src/gui/include/gui/gui.h"),
        ("rsz/Resizer.hh", "src/rsz/include/rsz/Resizer.hh"),
        ("drt/TritonRoute.h", "src/drt/include/drt/TritonRoute.h"),
        ("ant/AntennaChecker.hh", "src/ant/include/ant/AntennaChecker.hh"),
        ("gpl/Replace.h", "src/gpl/include/gpl/Replace.h"),
        ("pdn/PdnGen.hh", "src/pdn/include/pdn/PdnGen.hh"),
        ("cts/TritonCTS.h", "src/cts/include/cts/TritonCTS.h"),
    ]

    # Flatten tree for positioning
    # Layout: vertical tree with the chain on the left, consumers fanning right
    fig, ax = plt.subplots(figsize=(28, 20))
    ax.set_xlim(-8, 22)
    ax.set_ylim(-3, 17)
    ax.axis("off")

    # Color map: red = high cost, green = low cost
    import matplotlib.colors as mcolors

    cmap = plt.cm.RdYlGn_r  # Red=high, Green=low

    def cost_color(cost_pct):
        return cmap(min(cost_pct / 70, 1.0))  # Normalize to ~70% max

    def draw_node(
        x, y, label, cost, pct, ext,
        fontsize=8, bold=False, label_side="left",
    ):
        color = cost_color(pct)
        size = max(800, min(4000, cost / 100))
        radius_pts = size**0.5 / 2
        ax.scatter(x, y, s=size, c=[color], edgecolors="black",
                   linewidths=0.8, zorder=5, alpha=0.85)
        weight = "bold" if bold else "normal"
        if label_side == "left":
            ax.annotate(
                f"{label}\n{cost / 1000:.0f}K ({pct:.0f}%) {ext} ext",
                (x, y),
                textcoords="offset points",
                xytext=(-radius_pts - 12, 0),
                ha="right",
                va="center",
                fontsize=fontsize,
                fontweight=weight,
            )
        else:
            ax.annotate(
                f"{label}\n{cost / 1000:.0f}K ({pct:.0f}%) {ext} ext",
                (x, y),
                textcoords="offset points",
                xytext=(radius_pts + 12, 0),
                ha="left",
                va="center",
                fontsize=fontsize,
                fontweight=weight,
            )

    def draw_edge(x1, y1, x2, y2, style="-", color="#888888", lw=1.2):
        ax.annotate(
            "",
            xy=(x2, y2),
            xytext=(x1, y1),
            arrowprops=dict(
                arrowstyle="-|>",
                color=color,
                lw=lw,
                linestyle=style,
                connectionstyle="arc3,rad=0.1",
            ),
        )

    # Position the main chain vertically on the left
    chain = [
        ("utl/Logger.h", "src/utl/include/utl/Logger.h"),
        ("odb/isotropy.h", "src/odb/include/odb/isotropy.h"),
        ("odb/geom.h", "src/odb/include/odb/geom.h"),
        ("odb/dbTypes.h", "src/odb/include/odb/dbTypes.h"),
        ("odb/db.h", "src/odb/include/odb/db.h"),
    ]

    chain_x = 2.0
    chain_positions = {}
    for i, (label, path) in enumerate(chain):
        y = 14.5 - i * 2.8
        cost = rebuild_cost(path)
        pct = cost / total_cpp_loc * 100 if total_cpp_loc > 0 else 0
        ext = ext_cpp_count(path)
        draw_node(chain_x, y, label, cost, pct, ext, fontsize=18, bold=True, label_side="left")
        chain_positions[path] = (chain_x, y)
        if i > 0:
            prev_path = chain[i - 1][1]
            px, py = chain_positions[prev_path]
            draw_edge(px, py - 0.3, chain_x, y + 0.3, color="#CC0000", lw=2)

    # Metrics.h branching off Logger.h
    metrics_path = "src/utl/include/utl/Metrics.h"
    mx, my = chain_x - 2.2, 13.0
    cost = rebuild_cost(metrics_path)
    pct = cost / total_cpp_loc * 100 if total_cpp_loc > 0 else 0
    ext = ext_cpp_count(metrics_path)
    draw_node(mx, my, "utl/Metrics.h", cost, pct, ext, fontsize=16, label_side="left")
    lx, ly = chain_positions["src/utl/include/utl/Logger.h"]
    draw_edge(lx - 0.2, ly - 0.3, mx + 0.2, my + 0.3, color="#CC0000", lw=1.5)

    # db.h sub-headers on the left
    db_sub = [
        ("dbObject.h", "src/odb/include/odb/dbObject.h"),
        ("dbSet.h", "src/odb/include/odb/dbSet.h"),
        ("dbStream.h", "src/odb/include/odb/dbStream.h"),
        ("dbMatrix.h", "src/odb/include/odb/dbMatrix.h"),
    ]
    dbx, dby = chain_positions["src/odb/include/odb/db.h"]
    for i, (label, path) in enumerate(db_sub):
        sx = chain_x - 2.2
        sy = dby - 1.0 - i * 1.1
        cost = rebuild_cost(path)
        pct = cost / total_cpp_loc * 100 if total_cpp_loc > 0 else 0
        ext = ext_cpp_count(path)
        draw_node(sx, sy, label, cost, pct, ext, fontsize=14, label_side="left")
        draw_edge(dbx - 0.2, dby - 0.3, sx + 0.2, sy + 0.2, color="#888888", lw=0.8)

    # Consumer modules fanning out to the right
    cx_start = 7.0
    cy_top = 15.0
    for i, (label, path) in enumerate(consumers):
        cx = cx_start + (i % 2) * 4.0
        cy = cy_top - i * 1.2
        cost = rebuild_cost(path)
        pct = cost / total_cpp_loc * 100 if total_cpp_loc > 0 else 0
        ext = ext_cpp_count(path)
        draw_node(cx, cy, label, cost, pct, ext, fontsize=16, label_side="right")

        # Connect to the chain node they primarily depend on
        # dbSta/dbNetwork connect to db.h; others connect to Logger.h or db.h
        if "dbSta" in path or "dbNetwork" in path:
            tx, ty = chain_positions["src/odb/include/odb/db.h"]
        elif "gui" in label or "stt" in label:
            tx, ty = chain_positions["src/odb/include/odb/geom.h"]
        else:
            tx, ty = chain_positions["src/odb/include/odb/db.h"]
        draw_edge(tx + 0.3, ty, cx - 0.3, cy, style="--", color="#4488CC", lw=0.8)

    # Title and legend
    ax.set_title(
        "OpenROAD Include Dependency Tree\n"
        "Node size & color = transitive rebuild cost (red=high, green=low)",
        fontsize=20,
        fontweight="bold",
        pad=20,
    )

    # Add legend for the chain
    ax.annotate(
        "Critical chain\n(red arrows)",
        xy=(0.5, 15.5),
        fontsize=14,
        color="#CC0000",
        fontstyle="italic",
    )
    ax.annotate(
        "Consumer headers\n(blue dashed)",
        xy=(6.0, 16.2),
        fontsize=14,
        color="#4488CC",
        fontstyle="italic",
    )

    # Color bar
    sm = plt.cm.ScalarMappable(cmap=cmap, norm=mcolors.Normalize(0, 70))
    sm.set_array([])
    cbar = fig.colorbar(sm, ax=ax, shrink=0.4, aspect=20, pad=0.02)
    cbar.set_label("Rebuild cost (% of project LOC)", fontsize=14)

    fig.tight_layout()
    fig.savefig(output_path, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"Dependency tree plot saved to {output_path}")


def generate_dep_tree_text(
    cpp_graph, reverse_graph, loc, source_files
):
    """Generate ASCII dependency tree text for README injection."""
    total_cpp_loc = sum(
        loc.get(str(f), 0)
        for f in source_files
        if f.suffix in {".cpp", ".cc"}
    )
    cpp_files = {
        str(f)
        for f in source_files
        if f.suffix in {".cpp", ".cc"}
    }

    def rebuild_cost(header):
        visited = set()
        queue = collections.deque([header])
        total = 0
        while queue:
            node = queue.popleft()
            if node in visited:
                continue
            visited.add(node)
            if node in cpp_files:
                total += loc.get(node, 0)
            for dep in reverse_graph.get(node, set()):
                if dep not in visited:
                    queue.append(dep)
        return total

    def ext_cpp(header):
        visited = set()
        queue = collections.deque([header])
        count = 0
        while queue:
            node = queue.popleft()
            if node in visited:
                continue
            visited.add(node)
            if node in cpp_files and not node.startswith("src/odb/"):
                count += 1
            for dep in reverse_graph.get(node, set()):
                if dep not in visited:
                    queue.append(dep)
        return count

    def fmt(label, path, prefix=""):
        cost = rebuild_cost(path)
        pct = cost / total_cpp_loc * 100 if total_cpp_loc else 0
        ext = ext_cpp(path)
        pad = 60 - len(prefix) - len(label)
        dots = " " + "\u00b7" * max(pad, 2) + " " if pad > 2 else " "
        return (
            f"{prefix}{label}{dots}"
            f"{cost:>8,} LOC ({pct:>5.1f}%)  "
            f"{ext} external .cpp"
        )

    lines = []
    lines.append(
        f"Total project .cpp LOC: {total_cpp_loc:,}"
    )
    lines.append("")

    # Main chain
    chain = [
        ("utl/Logger.h",
         "src/utl/include/utl/Logger.h"),
        ("odb/isotropy.h",
         "src/odb/include/odb/isotropy.h"),
        ("odb/geom.h",
         "src/odb/include/odb/geom.h"),
        ("odb/dbTypes.h",
         "src/odb/include/odb/dbTypes.h"),
        ("odb/db.h",
         "src/odb/include/odb/db.h"),
    ]

    lines.append(fmt(chain[0][0], chain[0][1]))
    lines.append("\u2502")
    lines.append(
        fmt(
            "utl/Metrics.h",
            "src/utl/include/utl/Metrics.h",
            "\u251c\u2500\u2500 ",
        )
    )
    lines.append("\u2502")

    # Nested chain
    indent_chars = [
        ("\u251c\u2500\u2500 ", "\u2502   "),
        ("\u2514\u2500\u2500 ", "    "),
        ("\u2514\u2500\u2500 ", "    "),
        ("\u2514\u2500\u2500 ", "    "),
    ]
    prefix_stack = ""
    for i, (label, path) in enumerate(chain[1:]):
        pfx_char, cont = indent_chars[i]
        lines.append(fmt(label, path, prefix_stack + pfx_char))
        prefix_stack += cont

    # db.h sub-headers
    db_sub = [
        ("odb/dbObject.h",
         "src/odb/include/odb/dbObject.h"),
        ("odb/dbSet.h",
         "src/odb/include/odb/dbSet.h"),
        ("odb/dbStream.h",
         "src/odb/include/odb/dbStream.h"),
        ("odb/dbMatrix.h",
         "src/odb/include/odb/dbMatrix.h"),
    ]
    for j, (label, path) in enumerate(db_sub):
        is_last = j == len(db_sub) - 1
        pfx = "\u2514\u2500\u2500 " if is_last else "\u251c\u2500\u2500 "
        lines.append(
            fmt(label, path, prefix_stack + "    " + pfx)
        )

    lines.append("")

    # Consumer module headers
    lines.append("Consumer module headers:")
    consumers = [
        ("db_sta/dbSta.hh",
         "src/dbSta/include/db_sta/dbSta.hh"),
        ("db_sta/dbNetwork.hh",
         "src/dbSta/include/db_sta/dbNetwork.hh"),
        ("stt/SteinerTreeBuilder.h",
         "src/stt/include/stt/SteinerTreeBuilder.h"),
        ("grt/GlobalRouter.h",
         "src/grt/include/grt/GlobalRouter.h"),
        ("dpl/Opendp.h",
         "src/dpl/include/dpl/Opendp.h"),
        ("gui/gui.h",
         "src/gui/include/gui/gui.h"),
        ("rsz/Resizer.hh",
         "src/rsz/include/rsz/Resizer.hh"),
        ("drt/TritonRoute.h",
         "src/drt/include/drt/TritonRoute.h"),
        ("ant/AntennaChecker.hh",
         "src/ant/include/ant/AntennaChecker.hh"),
        ("gpl/Replace.h",
         "src/gpl/include/gpl/Replace.h"),
        ("pdn/PdnGen.hh",
         "src/pdn/include/pdn/PdnGen.hh"),
        ("cts/TritonCTS.h",
         "src/cts/include/cts/TritonCTS.h"),
    ]
    for label, path in consumers:
        if path in cpp_graph:
            lines.append(fmt(label, path, "  "))

    return "\n".join(lines)


def generate_module_dep_text(cpp_graph):
    """Generate module-level dependency text."""
    module_deps = collections.defaultdict(set)
    for f, deps in cpp_graph.items():
        if not deps:
            continue
        parts = f.split("/")
        if len(parts) < 2 or parts[0] != "src":
            continue
        src_mod = parts[1]
        if src_mod == "cmake":
            continue
        for d in deps:
            dp = d.split("/")
            if len(dp) < 2 or dp[0] != "src":
                continue
            dst_mod = dp[1]
            if dst_mod != src_mod and dst_mod != "cmake":
                module_deps[src_mod].add(dst_mod)

    lines = []
    for mod in sorted(module_deps):
        deps = sorted(module_deps[mod])
        lines.append(
            f"{mod} \u2500\u2500\u2192 {', '.join(deps)}"
        )
    return "\n".join(lines)


def update_readme_section(readme_path, marker_name, text):
    """Inject auto-generated text into README.md between markers.

    Markers: <!-- {marker_name}_START --> and <!-- {marker_name}_END -->
    """
    start = f"<!-- {marker_name}_START -->"
    end = f"<!-- {marker_name}_END -->"

    try:
        content = readme_path.read_text()
    except FileNotFoundError:
        return

    si = content.find(start)
    ei = content.find(end)
    if si == -1 or ei == -1:
        return

    new_content = (
        content[: si + len(start)]
        + "\n```\n"
        + text
        + "\n```\n"
        + content[ei:]
    )
    readme_path.write_text(new_content)
    print(f"Updated {marker_name} in {readme_path}")


def generate_pruning_plot(header_costs, total_project_loc, output_path, top_n=30):
    """Generate a sorted pruning chart: cumulative rebuild LOC reduction
    as worst headers are pruned one by one.

    X-axis: number of headers pruned (1, 2, 3, ...)
    Y-axis: total transitive rebuild cost (LOC) of the worst header set
    """
    ranked = sorted(header_costs.items(), key=lambda x: -x[1])[:top_n]
    if not ranked:
        return

    counts = list(range(1, len(ranked) + 1))
    costs = [cost for _, cost in ranked]
    # Cumulative: cost if you pruned top-1, top-2, etc.
    # Each bar shows the individual header's cost
    cumulative_costs = []
    running = 0
    for cost in costs:
        running += cost
        cumulative_costs.append(running)

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(18, 8))

    # Left plot: individual header rebuild costs (bar chart)
    names = [Path(h).name for h, _ in ranked]
    ax1.barh(range(len(names)), [c / 1000 for c in costs], color="#2196F3", alpha=0.8)
    ax1.set_yticks(range(len(names)))
    ax1.set_yticklabels(names, fontsize=7)
    ax1.set_xlabel("Transitive Rebuild Cost (KLOC)")
    ax1.set_title(f"Top {top_n} Worst Headers by Rebuild Cost")
    ax1.invert_yaxis()
    ax1.grid(True, alpha=0.3, axis="x")

    # Add full path as annotation for top 10
    for i, (h, cost) in enumerate(ranked[:10]):
        ax1.annotate(
            h,
            xy=(cost / 1000, i),
            xytext=(5, 0),
            textcoords="offset points",
            fontsize=5,
            alpha=0.6,
            va="center",
        )

    # Right plot: cumulative savings curve
    ax2.plot(counts, [c / 1000 for c in cumulative_costs], "o-", color="#E91E63", markersize=4)
    ax2.fill_between(counts, [c / 1000 for c in cumulative_costs], alpha=0.2, color="#E91E63")
    ax2.set_xlabel("Number of Headers Pruned (sorted by impact)")
    ax2.set_ylabel("Cumulative Rebuild Cost Removed (KLOC)")
    ax2.set_title("Cumulative Savings from Pruning Worst Headers")
    ax2.grid(True, alpha=0.3)

    # Add percentage axis on right
    ax3 = ax2.twinx()
    ax3.set_ylim(0, ax2.get_ylim()[1] / (total_project_loc / 1000) * 100)
    ax3.set_ylabel("% of Total Project LOC")

    fig.suptitle("Header Pruning Impact Analysis", fontsize=14, y=1.02)
    fig.tight_layout()
    fig.savefig(output_path, dpi=150, bbox_inches="tight")
    plt.close(fig)
    print(f"Pruning plot saved to {output_path}")


# ---------------------------------------------------------------------------
# YAML output
# ---------------------------------------------------------------------------


def save_yaml(data, path):
    """Save data to YAML file."""
    with open(path, "w") as f:
        yaml.dump(data, f, default_flow_style=False, sort_keys=True)
    print(f"Saved {path}")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------


def main():
    parser = argparse.ArgumentParser(
        description="Build dependency analysis and compile-time estimation"
    )
    parser.add_argument(
        "--months",
        type=int,
        default=3,
        help="Number of months of git history to analyze (default: 3)",
    )
    parser.add_argument(
        "--output-dir",
        type=str,
        default=None,
        help="Output directory (default: tools/stats/ in project root)",
    )
    parser.add_argument(
        "--skip-bazel",
        action="store_true",
        help="Skip Bazel dependency extraction (faster, C++ only)",
    )
    parser.add_argument(
        "--window",
        type=int,
        default=20,
        help="Rolling average window size for plot smoothing (default: 20)",
    )
    args = parser.parse_args()

    root = find_project_root()
    output_dir = Path(args.output_dir) if args.output_dir else root / "tools" / "stats"
    output_dir.mkdir(parents=True, exist_ok=True)

    # --- Step 1: Build C++ include graph ---
    print("Building C++ include dependency graph...")
    cpp_graph, source_files = build_cpp_include_graph(root)
    total_edges = sum(len(v) for v in cpp_graph.values())
    print(f"  {len(source_files)} source files, {total_edges} include edges")

    save_yaml(cpp_graph, output_dir / "cpp_deps.yaml")

    # --- Step 2: Build Bazel dependency graph ---
    bazel_graph = {}
    if not args.skip_bazel:
        bazel_graph = build_bazel_graph(root)
        save_yaml(bazel_graph, output_dir / "bazel_deps.yaml")

    # --- Step 3: Compute LOC and reverse graph ---
    print("Counting lines of code...")
    loc = count_loc(root, source_files)
    total_loc = sum(loc.values())
    print(f"  Total: {total_loc:,} lines across {len(loc)} files")

    reverse_graph = build_reverse_graph(cpp_graph)

    # --- Step 4: Find worst headers ---
    print("Computing header rebuild costs...")
    header_costs = compute_header_rebuild_costs(reverse_graph, loc, source_files)
    worst = rank_worst_headers(header_costs, top_n=10)

    print("\nTop 10 worst headers (by transitive rebuild cost):")
    print(f"{'Rank':<5} {'Header':<60} {'Rebuild LOC':>12}")
    print("-" * 80)
    for i, (h, cost) in enumerate(worst, 1):
        print(f"{i:<5} {h:<60} {cost:>12,}")

    # Headers to prune for the scenarios
    prune_candidates = [h for h, _ in worst[:5]]

    # --- Step 5: Analyze git history ---
    print(f"\nAnalyzing git history (last {args.months} months)...")
    commits = get_commits(root, args.months)
    print(f"  Found {len(commits)} non-merge commits")

    if not commits:
        print("No commits found in the specified range.")
        return

    dates = []
    series = {
        "C++ full rebuild": [],
        "Bazel functional units": [],
    }
    for i in range(5):
        pruned_names = [Path(p).name for p in prune_candidates[: i + 1]]
        series[f"Pruned top-{i + 1} ({', '.join(pruned_names)})"] = []

    for idx, (commit_hash, timestamp) in enumerate(reversed(commits)):
        if idx % 100 == 0:
            print(f"  Processing commit {idx + 1}/{len(commits)}...", end="\r")

        dt = datetime.datetime.fromtimestamp(timestamp)
        dates.append(dt)

        changed = get_changed_files(root, commit_hash)

        # Full C++ rebuild cost
        full_loc = compute_rebuild_loc(changed, reverse_graph, loc)
        series["C++ full rebuild"].append(full_loc)

        # Bazel affected modules
        modules = compute_bazel_affected_modules(changed, bazel_graph)
        series["Bazel functional units"].append(len(modules))

        # Pruned scenarios
        for i in range(5):
            pruned_set = set(prune_candidates[: i + 1])
            pruned_loc = compute_rebuild_loc(
                changed, reverse_graph, loc, pruned_headers=pruned_set
            )
            pruned_names = [Path(p).name for p in prune_candidates[: i + 1]]
            series[f"Pruned top-{i + 1} ({', '.join(pruned_names)})"].append(
                pruned_loc
            )

    print(f"  Processed {len(commits)} commits.           ")

    # --- Step 6: Generate plots ---
    print("Generating plots...")
    generate_plot(dates, series, output_dir / "compile_time.png")
    generate_pruning_plot(
        header_costs, total_loc, output_dir / "pruning_impact.png", top_n=30
    )
    generate_dep_tree_plot(
        cpp_graph, reverse_graph, loc, source_files,
        output_dir / "dep_tree.png",
    )

    # --- Step 7: Update README with auto-generated text ---
    readme_path = output_dir / "README.md"
    tree_text = generate_dep_tree_text(
        cpp_graph, reverse_graph, loc, source_files
    )
    update_readme_section(readme_path, "DEP_TREE", tree_text)
    mod_text = generate_module_dep_text(cpp_graph)
    update_readme_section(readme_path, "MODULE_DEPS", mod_text)

    # --- Summary statistics ---
    avg_full = sum(series["C++ full rebuild"]) / len(series["C++ full rebuild"])
    print(f"\nAverage rebuild cost per commit: {avg_full:,.0f} LOC")
    if series["Bazel functional units"]:
        avg_bazel = sum(series["Bazel functional units"]) / len(
            series["Bazel functional units"]
        )
        print(f"Average Bazel functional units affected: {avg_bazel:.1f}")

    for i in range(5):
        pruned_names = [Path(p).name for p in prune_candidates[: i + 1]]
        key = f"Pruned top-{i + 1} ({', '.join(pruned_names)})"
        avg = sum(series[key]) / len(series[key])
        savings = (1 - avg / avg_full) * 100 if avg_full > 0 else 0
        print(f"Pruning top-{i + 1} headers: {avg:,.0f} LOC avg ({savings:.1f}% reduction)")

    print(f"\nDone. Output in {output_dir}/")


if __name__ == "__main__":
    main()
