#!/usr/bin/env python3
"""
DRC Destroyer — Arcade game for routing visualization.

This is just a fun feature and has no practical value.

Reads an ODB database after routing, extracts DRC violations, routing wires,
log warnings, and timing slack, then generates a standalone HTML arcade game.

All of this was done automatically with zero programming. The source code
for the OpenROAD project is the user-interface to Claude.

Usage:
    python game.py --odb path/to/5_route.odb [--logs path/to/logs/] [-o output.html]
    python game.py --demo [-o output.html]
"""

import argparse
import json
import os
import random
import re
import subprocess
import sys
import webbrowser

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

WIRE_BUDGET = 3000

LAYER_COLORS = {
    "M1": "#4444ff",
    "M2": "#ff4444",
    "M3": "#44ff44",
    "M4": "#ffff44",
    "M5": "#ff44ff",
    "M6": "#44ffff",
    "M7": "#ff8844",
    "M8": "#8844ff",
    "M9": "#44ff88",
    "Pad": "#aaaaaa",
}

DRC_VILLAIN_NAMES = {
    "Short": "Short Circuit the Terrible",
    "CutSpcTbl": "Cut Spacing the Merciless",
    "eolKeepOut": "End-of-Line the Forbidden",
    "Metal Spacing": "Metal Spacing the Crusher",
    "EOL": "EOL the Edgelord",
    "Recheck": "Recheck the Persistent",
}

WARNING_PATTERNS = [
    (
        r"(\d+) input ports missing set_input_delay",
        "Lord Set_Input_Delay",
        "{n} ports cry out in silence!",
    ),
    (
        r"(\d+) output ports missing set_output_delay",
        "Baron Output_Delay",
        "{n} outputs lost in time!",
    ),
    (
        r"(\d+) unconstrained endpoints",
        "The Unconstrained {n}",
        "Free from all timing bonds!",
    ),
    (
        r"\[WARNING EST-0027\]",
        "Commander No_Parasitics",
        "Resistance is futile... literally!",
    ),
    (r"(\d+) floating nets", "The Floating {n}", "Untethered and dangerous!"),
    (r"only has one pin", "Dangling Reset", "A lone pin in the void!"),
    (r"No diode.*ANTENNACELL", "The Antenna Menace", "No diode can stop me!"),
]

# ---------------------------------------------------------------------------
# ODB Data Extraction
# ---------------------------------------------------------------------------


def extract_from_odb(odb_path):
    """Extract game data from an ODB file using the odb Python module."""
    import odb

    db = odb.dbDatabase.create()
    db.read(odb_path)
    chip = db.getChip()
    if chip is None:
        print(f"Error: Could not read chip from {odb_path}")
        return None
    block = chip.getBlock()
    if block is None:
        print(f"Error: Could not read block from {odb_path}")
        return None

    # Die area
    die = block.getDieArea()
    die_area = {
        "xMin": die.xMin(),
        "yMin": die.yMin(),
        "xMax": die.xMax(),
        "yMax": die.yMax(),
    }

    # DRC markers
    drc_markers = []
    for cat in block.getMarkerCategories():
        _collect_markers(cat, drc_markers)

    # Wire segments (sampled)
    wires = _extract_wires(block, odb)

    # Cell instances (for ticker)
    cell_names = set()
    for inst in block.getInsts():
        cell_names.add(inst.getMaster().getName())

    return {
        "die": die_area,
        "drc": drc_markers,
        "wires": wires,
        "cells": sorted(cell_names),
        "design_name": block.getName(),
    }


def _collect_markers(category, markers):
    """Recursively collect markers from a category hierarchy."""
    for marker in category.getMarkers():
        bbox = marker.getBBox()
        layer_obj = marker.getTechLayer()
        layer_name = layer_obj.getName() if layer_obj else "unknown"
        markers.append(
            {
                "type": category.getName(),
                "xMin": bbox.xMin(),
                "yMin": bbox.yMin(),
                "xMax": bbox.xMax(),
                "yMax": bbox.yMax(),
                "layer": layer_name,
            }
        )
    for subcat in category.getMarkerCategories():
        _collect_markers(subcat, markers)


def _extract_wires(block, odb):
    """Extract wire segments from all nets, sampled to WIRE_BUDGET."""
    all_segments = []
    decoder = odb.dbWireDecoder()

    for net in block.getNets():
        wire = net.getWire()
        if wire is None:
            continue
        decoder.begin(wire)
        current_layer = None
        last_x, last_y = None, None

        while True:
            op = decoder.next()
            if op == odb.dbWireDecoder.END_DECODE:
                break
            elif op == odb.dbWireDecoder.PATH:
                current_layer = decoder.getLayer()
                last_x, last_y = None, None
            elif op == odb.dbWireDecoder.JUNCTION:
                jid = decoder.getJunctionValue()
                pt = wire.getCoord(jid)
                last_x, last_y = pt.x(), pt.y()
            elif op == odb.dbWireDecoder.POINT:
                pt = decoder.getPoint()
                x, y = pt[0], pt[1]
                if last_x is not None and current_layer is not None:
                    all_segments.append(
                        {
                            "layer": current_layer.getName(),
                            "x1": last_x,
                            "y1": last_y,
                            "x2": x,
                            "y2": y,
                        }
                    )
                last_x, last_y = x, y
            elif op == odb.dbWireDecoder.POINT_EXT:
                pt = decoder.getPoint_ext()
                x, y = pt[0], pt[1]
                if last_x is not None and current_layer is not None:
                    all_segments.append(
                        {
                            "layer": current_layer.getName(),
                            "x1": last_x,
                            "y1": last_y,
                            "x2": x,
                            "y2": y,
                        }
                    )
                last_x, last_y = x, y

    # Sample if over budget
    if len(all_segments) > WIRE_BUDGET:
        all_segments = random.sample(all_segments, WIRE_BUDGET)

    return all_segments


# ---------------------------------------------------------------------------
# Log Parsing
# ---------------------------------------------------------------------------


def parse_log_warnings(log_dir):
    """Parse log files for warnings and return villain data."""
    villains = []
    if not log_dir or not os.path.isdir(log_dir):
        return villains

    seen = set()
    for root, _dirs, files in os.walk(log_dir):
        for fname in sorted(files):
            if not fname.endswith(".log"):
                continue
            fpath = os.path.join(root, fname)
            try:
                with open(fpath) as f:
                    text = f.read()
            except OSError:
                continue

            for pattern, name_tpl, quote_tpl in WARNING_PATTERNS:
                for m in re.finditer(pattern, text):
                    n = m.group(1) if m.lastindex and m.lastindex >= 1 else ""
                    name = name_tpl.format(n=n)
                    if name in seen:
                        continue
                    seen.add(name)
                    quote = quote_tpl.format(n=n)
                    villains.append({"name": name, "quote": quote})

    return villains


def parse_drc_log(log_dir):
    """Parse route log for DRC violation table from iteration 0."""
    drc_villains = []
    if not log_dir:
        return drc_villains

    route_log = None
    for root, _dirs, files in os.walk(log_dir):
        for fname in files:
            if "route" in fname and fname.endswith(".log"):
                route_log = os.path.join(root, fname)
                break
        if route_log:
            break

    if not route_log:
        return drc_villains

    try:
        with open(route_log) as f:
            text = f.read()
    except OSError:
        return drc_villains

    # Parse violation table: "Viol/Layer  M2  M3 ..." followed by rows
    table_match = re.search(
        r"Number of violations = (\d+)\.\nViol/Layer\s+(.*?)\n((?:.*\n)*?)(?:\[INFO|Total wire)",
        text,
    )
    if table_match:
        total = int(table_match.group(1))
        layers = table_match.group(2).split()
        for line in table_match.group(3).strip().split("\n"):
            parts = line.split()
            if len(parts) < 2:
                continue
            # Violation type may have spaces (e.g. "Metal Spacing")
            # Count columns from the right based on number of layers
            counts = parts[-len(layers) :]
            vtype = " ".join(parts[: -len(layers)])
            try:
                total_count = sum(int(c) for c in counts)
            except ValueError:
                continue
            if total_count == 0:
                continue
            vname = DRC_VILLAIN_NAMES.get(vtype, f"{vtype} the Villainous")
            active_layers = [l for l, c in zip(layers, counts) if c != "0"]
            drc_villains.append(
                {
                    "name": vname,
                    "type": vtype,
                    "count": total_count,
                    "layers": active_layers,
                }
            )

    return drc_villains


def parse_timing(log_dir):
    """Parse metadata.json for timing information."""
    timing = {"setup_ws": None, "hold_ws": None, "fmax": None}
    if not log_dir:
        return timing

    # Look for metadata.json in reports dir (sibling to logs)
    for search_dir in [log_dir, os.path.dirname(log_dir)]:
        for root, _dirs, files in os.walk(search_dir):
            if "metadata.json" in files:
                try:
                    with open(os.path.join(root, "metadata.json")) as f:
                        meta = json.load(f)
                    timing["setup_ws"] = meta.get("finish__timing__setup__ws")
                    timing["hold_ws"] = meta.get("finish__timing__hold__ws")
                    timing["fmax"] = meta.get("finish__timing__fmax")
                    return timing
                except (OSError, json.JSONDecodeError):
                    pass

    return timing


# ---------------------------------------------------------------------------
# Demo Data Generation
# ---------------------------------------------------------------------------


def generate_demo_data():
    """Generate fake game data for demo mode."""
    die = {"xMin": 0, "yMin": 0, "xMax": 100000, "yMax": 100000}

    # Fake DRC markers
    drc = []
    for i in range(25):
        vtype = random.choice(
            ["Short", "CutSpcTbl", "eolKeepOut", "Metal Spacing", "EOL"]
        )
        x = random.randint(5000, 95000)
        y = random.randint(5000, 95000)
        layer = random.choice(["M2", "M3", "M4", "M5"])
        drc.append(
            {
                "type": vtype,
                "xMin": x,
                "yMin": y,
                "xMax": x + 500,
                "yMax": y + 500,
                "layer": layer,
            }
        )

    # Fake wires
    layers = ["M1", "M2", "M3", "M4", "M5"]
    wires = []
    for _ in range(2000):
        layer = random.choice(layers)
        x1 = random.randint(1000, 99000)
        y1 = random.randint(1000, 99000)
        if layer in ("M1", "M3", "M5"):
            x2, y2 = x1 + random.randint(500, 5000), y1
        else:
            x2, y2 = x1, y1 + random.randint(500, 5000)
        wires.append({"layer": layer, "x1": x1, "y1": y1, "x2": x2, "y2": y2})

    cells = [
        "BUFx2_ASAP7_75t_R",
        "INVx1_ASAP7_75t_R",
        "AND2x2_ASAP7_75t_R",
        "DFFHQNx1_ASAP7_75t_R",
        "AO21x1_ASAP7_75t_R",
        "OA21x2_ASAP7_75t_R",
        "HAxp5_ASAP7_75t_R",
        "FAx1_ASAP7_75t_R",
        "XNOR2x1_ASAP7_75t_R",
    ]

    return {
        "die": die,
        "drc": drc,
        "wires": wires,
        "cells": cells,
        "design_name": "DemoArray",
    }


# ---------------------------------------------------------------------------
# Intro Text Generation
# ---------------------------------------------------------------------------


def build_intro_text(design_name, drc_markers, warning_villains, drc_villains, timing):
    """Build the Star Wars/Spaceballs parody intro text."""
    lines = [
        "",
        "A LONG TIME AGO",
        "IN A FAB FAR, FAR AWAY...",
        "",
        "",
        "DRC DESTROYER",
        "THE ROUTING STRIKES BACK",
        "",
        "",
    ]

    total_drc = len(drc_markers)
    if total_drc > 0:
        lines.append(f"The {design_name} galaxy is under siege.")
        lines.append(f"{total_drc} DRC violations threaten the very")
        lines.append("fabric of silicon spacetime.")
        lines.append("")
    else:
        lines.append(f"The {design_name} galaxy is at peace...")
        lines.append("But peace breeds complacency.")
        lines.append("")
        lines.append("TRAINING EXERCISE INITIATED")
        lines.append("For the virtuous saviours of the galaxy,")
        lines.append("simulated threats have been deployed.")
        lines.append("")

    if drc_villains:
        lines.append("Leading the assault:")
        lines.append("")
        for v in drc_villains[:4]:
            lines.append(v["name"].upper())
            layer_str = ", ".join(v.get("layers", []))
            lines.append(f"with {v['count']} {v['type']}s on {layer_str}...")
            lines.append("")

    if warning_villains:
        lines.append("From the shadows emerge:")
        lines.append("")
        for v in warning_villains[:4]:
            lines.append(v["name"].upper())
            lines.append(f'"{v["quote"]}"')
            lines.append("")

    if timing:
        ws = timing.get("setup_ws")
        if ws is not None and ws < 0:
            lines.append(f"NEGATIVE SLACK THE DESTROYER")
            lines.append(f"wields {ws:.1f}ps of pure destruction!")
            lines.append("")
        elif ws is not None:
            lines.append(f"The Slack is with you: +{ws:.1f}ps")
            fmax = timing.get("fmax")
            if fmax:
                lines.append(f"Fmax: {fmax / 1e9:.1f} GHz")
            lines.append("")

    lines.extend(
        [
            "Only you, brave engineer, can restore",
            "order to the routing grid.",
            "",
            "This game was created automatically",
            "with zero programming. The source code",
            "of OpenROAD is the user-interface to Claude.",
            "",
            "Do what you need for YOUR chip.",
            "The EDA build flow is now democratized.",
            "",
            "But beware - this is just a fun feature",
            "and has no practical value.",
            "",
            "May the Slack be with you.",
            "",
        ]
    )

    return lines


# ---------------------------------------------------------------------------
# HTML Game Generation
# ---------------------------------------------------------------------------


def generate_html(game_data, intro_lines, warning_villains, output_path):
    """Generate the self-contained HTML game file.

    Reads drc_destroyer.js template from the same directory and injects
    game data as JSON constants into a standalone HTML file.
    """
    script_dir = os.path.dirname(os.path.abspath(__file__))
    js_path = os.path.join(script_dir, "drc_destroyer.js")

    with open(js_path) as f:
        js_code = f.read()

    # Build the data injection block
    data_block = (
        f"const GAME_DATA = {json.dumps(game_data)};\n"
        f"const INTRO_LINES = {json.dumps(intro_lines)};\n"
        f"const WARNING_VILLAINS = {json.dumps(warning_villains)};\n"
        f"const LAYER_COLORS = {json.dumps(LAYER_COLORS)};\n"
    )

    html = _HTML_SHELL.replace("/* __INJECTED_DATA__ */", data_block)
    html = html.replace("/* __INJECTED_GAME_JS__ */", js_code)

    with open(output_path, "w") as f:
        f.write(html)

    return output_path


_HTML_SHELL = """<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>DRC Destroyer</title>
<style>
* { margin: 0; padding: 0; box-sizing: border-box; }
body { background: #000; overflow: hidden; display: flex;
       justify-content: center; align-items: center; height: 100vh; }
canvas { display: block; image-rendering: pixelated; }
</style>
</head>
<body>
<canvas id="c"></canvas>
<script>
/* __INJECTED_DATA__ */
/* __INJECTED_GAME_JS__ */
</script>
</body>
</html>
"""

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------


def main():
    parser = argparse.ArgumentParser(
        description="DRC Destroyer — generate arcade game HTML"
    )
    parser.add_argument("--odb", help="Path to routed ODB file")
    parser.add_argument("--logs", help="Path to log directory")
    parser.add_argument("--demo", action="store_true", help="Use demo data")
    parser.add_argument(
        "-o", "--output", default="/tmp/drc_destroyer.html", help="Output HTML path"
    )
    parser.add_argument(
        "--no-browser", action="store_true", help="Don't open browser automatically"
    )
    args = parser.parse_args()

    # Extract data
    if args.demo or not args.odb:
        print("DRC Destroyer — using demo data")
        game_data = generate_demo_data()
        warning_villains = [
            {"name": "Lord Set_Input_Delay", "quote": "1025 ports cry out in silence!"},
            {"name": "The Unconstrained 732", "quote": "Free from all timing bonds!"},
            {"name": "The Antenna Menace", "quote": "No diode can stop me!"},
        ]
        drc_villains = [
            {
                "name": "Short Circuit the Terrible",
                "type": "Short",
                "count": 165,
                "layers": ["M2", "M3", "M4", "M5"],
            },
        ]
        timing = {"setup_ws": 11.1, "hold_ws": 27.2, "fmax": 11.28e9}
    else:
        print(f"DRC Destroyer — reading ODB: {args.odb}")
        game_data = extract_from_odb(args.odb)
        if game_data is None:
            print("Failed to read ODB, falling back to demo data")
            game_data = generate_demo_data()

        warning_villains = parse_log_warnings(args.logs)
        drc_villains = parse_drc_log(args.logs)
        timing = parse_timing(args.logs)

    # Build intro
    intro_lines = build_intro_text(
        game_data["design_name"],
        game_data["drc"],
        warning_villains,
        drc_villains,
        timing,
    )

    # Generate HTML
    output = generate_html(game_data, intro_lines, warning_villains, args.output)
    print(f"\nDRC Destroyer game saved to: {output}")
    print("This is just a fun feature and has no practical value.")

    if not args.no_browser:
        print("Opening browser...")
        webbrowser.open(f"file://{os.path.abspath(output)}")


if __name__ == "__main__":
    main()
