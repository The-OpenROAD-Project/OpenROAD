#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors
"""Generate SVG railroad diagrams from the LEF and DEF Bison grammar files.

Downloads ebnf-convert and rr (Railroad Diagram Generator) from Maven Central
into doc/tools/ on first use (the JARs are not committed to the repository).
Requires Java 11 or later on PATH.

Usage:
    python3 src/odb/doc/generate_railroad_diagrams.py lef   # LEF only
    python3 src/odb/doc/generate_railroad_diagrams.py def   # DEF only
    python3 src/odb/doc/generate_railroad_diagrams.py all   # both
"""

import os
import shutil
import subprocess
import sys
import tempfile
import urllib.request
import zipfile
from pathlib import Path

SCRIPT_DIR = Path(__file__).parent.resolve()
REPO_ROOT = SCRIPT_DIR.parent.parent.parent  # doc/ -> odb/ -> src/ -> repo root

TOOLS_DIR = SCRIPT_DIR / "tools"

EBNF_WAR = TOOLS_DIR / "ebnf-convert.war"
RR_WAR = TOOLS_DIR / "rr.war"

# Maven Central coordinates — update versions here when upstream releases new ones.
EBNF_VERSION = "0.73"
RR_VERSION = "2.6"
_MVN = "https://repo1.maven.org/maven2/de/bottlecaps"
EBNF_URL = f"{_MVN}/ebnf-convert/{EBNF_VERSION}/ebnf-convert-{EBNF_VERSION}.war"
RR_URL = f"{_MVN}/rr/{RR_VERSION}/rr-{RR_VERSION}.war"

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


def _download(url: str, dest: Path) -> None:
    print(f"  Downloading {dest.name} from Maven Central…")
    dest.parent.mkdir(parents=True, exist_ok=True)
    with urllib.request.urlopen(url) as response, open(dest, "wb") as out:
        shutil.copyfileobj(response, out)
    print(f"  Saved {dest}")


def ensure_tools() -> None:
    """Check that the required WAR files exist in the tools directory."""
    missing = []
    if not EBNF_WAR.exists():
        missing.append(f"  ebnf-convert.war  (build from: https://github.com/GuntherRademacher/ebnf-convert)")
    if not RR_WAR.exists():
        missing.append(f"  rr.war            (download lib from Maven Central: de.bottlecaps.rr:rr-lib:2.6)")
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

        # Step 3 — Extract diagram/*.svg from the ZIP
        print(f"[{name}] Step 3: Extracting SVGs to {out_dir}…")
        if out_dir.exists():
            shutil.rmtree(out_dir)
        out_dir.mkdir(parents=True)

        count = 0
        with zipfile.ZipFile(zip_file) as z:
            for entry in z.namelist():
                if entry.startswith("diagram/") and entry.endswith(".svg"):
                    filename = os.path.basename(entry)
                    with z.open(entry) as src, open(out_dir / filename, "wb") as dst:
                        dst.write(src.read())
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
