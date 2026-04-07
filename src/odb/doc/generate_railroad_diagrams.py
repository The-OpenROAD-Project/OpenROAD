#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
"""Generate SVG railroad diagrams from the LEF and DEF Bison grammar files.

Requires the ebnf-convert.war and rr.war tools vendored in src/odb/doc/tools/.
Requires Java 11 or later on PATH.

Usage:
    python3 src/odb/doc/generate_railroad_diagrams.py         # both (default)
    python3 src/odb/doc/generate_railroad_diagrams.py lef     # LEF only
    python3 src/odb/doc/generate_railroad_diagrams.py def     # DEF only
    python3 src/odb/doc/generate_railroad_diagrams.py all     # both (explicit)
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path
import xml.etree.ElementTree as ET

SCRIPT_DIR = Path(__file__).parent.resolve()
REPO_ROOT = SCRIPT_DIR.parent.parent.parent  # doc/ -> odb/ -> src/ -> repo root

TOOLS_DIR = SCRIPT_DIR / "tools"

EBNF_WAR = TOOLS_DIR / "ebnf-convert.war"
RR_WAR = TOOLS_DIR / "rr.war"

GRAMMARS = {
    "lef": {
        "y_file": REPO_ROOT / "src" / "odb" / "src" / "lef" / "lef" / "lef.y",
        "out_dir": SCRIPT_DIR / "images" / "lef",
    },
    "def": {
        "y_file": REPO_ROOT / "src" / "odb" / "src" / "def" / "def" / "def.y",
        "out_dir": SCRIPT_DIR / "images" / "def",
    },
}

# Background colour applied to every generated SVG for visibility in both
# light and dark documentation themes.
DIAGRAM_BG = "#FFFCF0"


def fix_svg_background(svg_path: Path) -> None:
    """Inject a solid background colour into an SVG file."""

    def local_name(tag: str) -> str:
        if "}" in tag:
            return tag.split("}", 1)[1]
        return tag

    # Register existing namespace prefixes before serializing so ElementTree
    # keeps author-provided prefixes (for example, xlink) instead of ns0.
    for _, (prefix, uri) in ET.iterparse(svg_path, events=("start-ns",)):
        ET.register_namespace(prefix or "", uri)

    tree = ET.parse(svg_path)
    root = tree.getroot()

    bg_style = f"background: var(--diagram-bg, {DIAGRAM_BG});"
    existing_root_style = root.attrib.get("style", "").strip()
    if "background:" not in existing_root_style:
        root.attrib["style"] = (
            f"{existing_root_style} {bg_style}".strip()
            if existing_root_style
            else bg_style
        )

    style_node = None
    for child in list(root):
        if local_name(child.tag) == "style":
            style_node = child
            break

    css_var = f":root {{ --diagram-bg: {DIAGRAM_BG}; }}"
    if style_node is None:
        ns = ""
        if root.tag.startswith("{"):
            ns = root.tag.split("}", 1)[0] + "}"
        style_node = ET.Element(f"{ns}style")
        style_node.text = "\n  " + css_var + "\n"
        root.insert(0, style_node)
    else:
        existing_css = style_node.text or ""
        if "--diagram-bg" not in existing_css:
            style_node.text = (
                f"{existing_css.rstrip()}\n  {css_var}\n"
                if existing_css.strip()
                else f"\n  {css_var}\n"
            )

    tree.write(svg_path, encoding="utf-8", xml_declaration=True)


def ensure_tools() -> None:
    """Check that the required WAR files exist in the tools directory."""
    missing = []
    java_path = shutil.which("java")

    if java_path is None:
        missing.append(
            "  java              (install Java 11 or later and add it to PATH)"
        )
    else:
        version_result = subprocess.run(
            [java_path, "-version"],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            check=False,
        )
        if version_result.returncode != 0:
            missing.append("  java              (found in PATH but failed to execute)")
        else:
            match = re.search(r'"(\d+)(?:\.\d+)?', version_result.stdout)
            major = int(match.group(1)) if match else None
            if major is None or major < 11:
                missing.append("  java              (requires Java 11 or later)")

    if not EBNF_WAR.exists():
        missing.append(
            "  ebnf-convert.war  (build from: https://github.com/GuntherRademacher/ebnf-convert)"
        )
    if not RR_WAR.exists():
        missing.append(
            "  rr.war            (download lib from Maven Central: de.bottlecaps.rr:rr-lib:2.6)"
        )
    if missing:
        print("Error: Missing required tool(s) in src/odb/doc/tools/:", file=sys.stderr)
        for m in missing:
            print(m, file=sys.stderr)
        print("\nSee src/odb/doc/README.md for setup instructions.", file=sys.stderr)
        sys.exit(1)


def _extract_rr_deps(tmp_dir: str) -> list[str]:
    """Extract Saxon and xmlresolver JARs bundled inside ebnf-convert.war."""
    needed = [
        "WEB-INF/lib/Saxon-HE-12.9.jar",
        "WEB-INF/lib/xmlresolver-5.3.3.jar",
    ]
    with zipfile.ZipFile(EBNF_WAR) as z:
        for entry in needed:
            z.extract(entry, tmp_dir)
    return [os.path.join(tmp_dir, e) for e in needed]


def generate(name: str) -> None:
    cfg = GRAMMARS[name]
    y_file: Path = cfg["y_file"]
    out_dir: Path = cfg["out_dir"]

    if not y_file.exists():
        print(f"Error: grammar file not found: {y_file}", file=sys.stderr)
        sys.exit(1)

    tmp_dir = tempfile.mkdtemp(prefix=f"railroad-{name}-")
    ebnf_file = os.path.join(tmp_dir, f"{name}.ebnf")
    zip_file = os.path.join(tmp_dir, f"{name}_diagrams.zip")

    try:
        # Step 1 — Bison (.y) → W3C EBNF
        print(f"[{name}] Step 1: Converting {y_file.name} to EBNF…")
        with open(ebnf_file, "w", encoding="utf-8") as f:
            subprocess.run(
                ["java", "-jar", str(EBNF_WAR), str(y_file)],
                stdout=f,
                check=True,
            )

        # Step 2 — EBNF → ZIP of individual SVG files
        # rr.war has no Main-Class; Saxon (bundled in ebnf-convert.war) is required.
        print(f"[{name}] Step 2: Generating railroad SVGs…")
        dep_jars = _extract_rr_deps(tmp_dir)
        sep = os.pathsep
        classpath = sep.join([str(RR_WAR)] + dep_jars)
        subprocess.run(
            [
                "java",
                "-cp",
                classpath,
                "de.bottlecaps.railroad.Railroad",
                "-noembedded",
                f"-out:{zip_file}",
                ebnf_file,
            ],
            check=True,
        )

        # Step 3 — Extract diagram/*.svg from the ZIP and apply background fix
        print(f"[{name}] Step 3: Extracting SVGs to {out_dir}…")
        if out_dir.exists():
            shutil.rmtree(out_dir)
        out_dir.mkdir(parents=True)

        count = 0
        with zipfile.ZipFile(zip_file) as z:
            for entry in z.namelist():
                if entry.startswith("diagram/") and entry.endswith(".svg"):
                    filename = os.path.basename(entry)
                    dest = out_dir / filename
                    with z.open(entry) as src, open(dest, "wb") as dst:
                        dst.write(src.read())
                    fix_svg_background(dest)
                    count += 1

        print(f"[{name}] Done — {count} SVG files written to {out_dir}")

    finally:
        shutil.rmtree(tmp_dir, ignore_errors=True)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate SVG railroad diagrams from LEF/DEF Bison grammars."
    )
    parser.add_argument(
        "targets",
        nargs="*",
        choices=[*GRAMMARS.keys(), "all"],
        metavar="target",
        help="Grammar(s) to process: lef, def, or all (default: all)",
    )
    args = parser.parse_args()
    targets = args.targets if args.targets else ["all"]
    names = list(GRAMMARS.keys()) if "all" in targets else targets

    ensure_tools()
    for name in names:
        generate(name)


if __name__ == "__main__":
    main()
