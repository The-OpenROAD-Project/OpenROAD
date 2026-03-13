#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors
"""Generate SVG railroad diagrams from the LEF and DEF Bison grammar files.

Requires the ebnf-convert.war and rr.war tools vendored in src/odb/doc/tools/.
Requires Java 11 or later on PATH.

Usage:
    python3 src/odb/doc/generate_railroad_diagrams.py lef   # LEF only
    python3 src/odb/doc/generate_railroad_diagrams.py def   # DEF only
    python3 src/odb/doc/generate_railroad_diagrams.py all   # both
"""

import os
import re
import shutil
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path

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
    content = svg_path.read_text(encoding="utf-8")
    style_block = (
        "<style>\n"
        f"  :root {{ --diagram-bg: {DIAGRAM_BG}; }}\n"
        "</style>"
    )
    content = re.sub(r"(<svg[^>]*>)", r"\1" + style_block, content, count=1)
    content = re.sub(
        r"(<svg\b)([^>]*>)",
        rf'\1 style="background: var(--diagram-bg, {DIAGRAM_BG})"\2',
        content,
        count=1,
    )
    svg_path.write_text(content, encoding="utf-8")


def ensure_tools() -> None:
    """Check that the required WAR files exist in the tools directory."""
    missing = []
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
                "-cp", classpath,
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
    targets = sys.argv[1:]
    if not targets or targets[0] not in {"lef", "def", "all"}:
        print("Usage: generate_railroad_diagrams.py <lef|def|all>", file=sys.stderr)
        sys.exit(1)

    names = list(GRAMMARS.keys()) if targets[0] == "all" else [targets[0]]

    ensure_tools()
    for name in names:
        generate(name)


if __name__ == "__main__":
    main()
