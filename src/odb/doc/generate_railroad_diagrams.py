#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors
"""Generate railroad diagram SVGs from Bison grammar files.

Parses a Bison .y grammar file, extracts the top-level production rules, and
renders them as SVG railroad diagrams using the railroad-diagrams package.

Usage:
    pip install railroad-diagrams
    python3 generate_railroad_diagrams.py def  # regenerate DEF diagrams
    python3 generate_railroad_diagrams.py lef  # regenerate LEF diagrams
    python3 generate_railroad_diagrams.py all  # regenerate both
"""

import re
import sys
from pathlib import Path

try:
    import railroad
    from railroad import Diagram, Choice, Sequence, Terminal, NonTerminal, \
        Optional, ZeroOrMore, Comment
except ImportError:
    print("Install railroad-diagrams: pip install railroad-diagrams",
          file=sys.stderr)
    sys.exit(1)

SCRIPT_DIR = Path(__file__).parent
ODB_SRC = SCRIPT_DIR.parent / "src"

GRAMMARS = {
    "def": {
        "y_file":    ODB_SRC / "def" / "def" / "def.y",
        "out_dir":   SCRIPT_DIR / "img",
        "top_rules": [
            "def_file",
            "rule",
            "design_section",
            "comps_section",
            "nets_section",
            "snets_section",
            "pin_section",
        ],
    },
    "lef": {
        "y_file":    ODB_SRC / "lef" / "lef" / "lef.y",
        "out_dir":   SCRIPT_DIR / "img",
        "top_rules": [
            "lef_file",
            "rule",
            "layer_rule",
            "macro",
            "via",
            "site",
        ],
    },
}


def extract_rules(y_file: Path) -> dict[str, list[list[str]]]:
    """Extract grammar rules from a Bison .y file.

    Returns a dict mapping rule name -> list of alternatives,
    where each alternative is a list of symbol strings.
    Strips embedded C action blocks { ... }.
    """
    text = y_file.read_text(errors="replace")

    # Isolate the grammar section (after first %%)
    parts = re.split(r'^%%', text, maxsplit=1, flags=re.MULTILINE)
    if len(parts) < 2:
        raise ValueError(f"No %% separator found in {y_file}")
    grammar = parts[1]

    # Strip C action blocks (may be nested) naively by removing { ... }
    depth = 0
    cleaned = []
    i = 0
    while i < len(grammar):
        ch = grammar[i]
        if ch == '{':
            depth += 1
        elif ch == '}':
            depth -= 1
        elif depth == 0:
            cleaned.append(ch)
        i += 1
    grammar = ''.join(cleaned)

    rules: dict[str, list[list[str]]] = {}
    current_rule: str | None = None
    current_alt: list[str] = []

    for line in grammar.splitlines():
        line = line.strip()
        if not line or line.startswith("/*") or line.startswith("//"):
            continue

        # Rule head: "rule_name :"
        m = re.match(r'^(\w+)\s*:', line)
        if m:
            current_rule = m.group(1)
            rules.setdefault(current_rule, [])
            current_alt = []
            rest = line[m.end():].strip()
            if rest and rest != '|':
                tokens = rest.split()
                current_alt.extend(t for t in tokens if t not in (';', '|'))
            continue

        if current_rule is None:
            continue

        if line == ';':
            if current_alt:
                rules[current_rule].append(current_alt)
            current_rule = None
            current_alt = []
            continue

        tokens = line.split()
        for tok in tokens:
            if tok == '|':
                rules[current_rule].append(current_alt)
                current_alt = []
            elif tok == ';':
                rules[current_rule].append(current_alt)
                current_alt = []
                current_rule = None
                break
            else:
                current_alt.append(tok)

    return rules


def symbol_to_node(sym: str):
    """Convert a grammar symbol string to a railroad node."""
    # Quoted literals are terminals
    if sym.startswith(("'", '"')):
        return Terminal(sym.strip("'\""))
    # ALL_CAPS tokens are terminals (lex tokens)
    if sym.isupper() or re.match(r'^[A-Z][A-Z0-9_]+$', sym):
        return Terminal(sym)
    return NonTerminal(sym)


def build_diagram(alternatives: list[list[str]]) -> Diagram:
    """Build a railroad Diagram from a list of alternative symbol lists."""
    def alt_to_node(syms):
        nodes = [symbol_to_node(s) for s in syms if s]
        if not nodes:
            return Terminal('ε')
        if len(nodes) == 1:
            return nodes[0]
        return Sequence(*nodes)

    if not alternatives:
        return Diagram(Terminal('ε'))

    if len(alternatives) == 1:
        return Diagram(alt_to_node(alternatives[0]))

    # Limit to first 8 alternatives to keep SVG readable
    alts = alternatives[:8]
    if len(alternatives) > 8:
        alts.append(['...'])
    return Diagram(Choice(0, *[alt_to_node(a) for a in alts]))


def generate(grammar_name: str) -> None:
    cfg = GRAMMARS[grammar_name]
    y_file: Path = cfg["y_file"]
    out_dir: Path = cfg["out_dir"]
    top_rules: list[str] = cfg["top_rules"]

    if not y_file.exists():
        print(f"Grammar file not found: {y_file}", file=sys.stderr)
        sys.exit(1)

    out_dir.mkdir(parents=True, exist_ok=True)

    print(f"Parsing {y_file.name} ...")
    rules = extract_rules(y_file)

    for rule_name in top_rules:
        if rule_name not in rules:
            print(f"  Warning: rule '{rule_name}' not found, skipping")
            continue

        diagram = build_diagram(rules[rule_name])
        out_svg = out_dir / f"{grammar_name}_{rule_name}.svg"

        with out_svg.open("w") as f:
            diagram.writeSvg(f.write)

        print(f"  Written {out_svg.relative_to(SCRIPT_DIR)}")


def main():
    targets = sys.argv[1:] if len(sys.argv) > 1 else ["all"]
    if "all" in targets:
        targets = list(GRAMMARS.keys())

    for t in targets:
        if t not in GRAMMARS:
            print(f"Unknown grammar '{t}'. Choose from: {list(GRAMMARS)}")
            sys.exit(1)
        generate(t)


if __name__ == "__main__":
    main()
