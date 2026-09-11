#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# embed_report_assets.py strips ES module syntax with regexes to concatenate the
# widget sources into one <script type="module">.  A statement it misses is a
# syntax error that costs every widget; one it over-strips deletes code in
# silence.  Both have happened, so the shapes are pinned here.

import importlib.util
import os
import sys

THIS_DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPT = os.path.join(THIS_DIR, "..", "src", "embed_report_assets.py")

spec = importlib.util.spec_from_file_location("embed_report_assets", SCRIPT)
embed = importlib.util.module_from_spec(spec)
spec.loader.exec_module(embed)

failures = []


def check(name, condition, detail=""):
    if not condition:
        failures.append(f"{name}: {detail}" if detail else name)


def check_raises(name, call):
    try:
        call()
    except SystemExit:
        return
    failures.append(f"{name}: expected SystemExit, none raised")


def stripped_imports():
    """An import, in every shape the sources use, goes away with nothing else."""
    cases = {
        "plain": "import { X } from './x.js';\nconst kept = 1;\n",
        "side effect": "import './x.js';\nconst kept = 1;\n",
        "default": "import X from './x.js';\nconst kept = 1;\n",
        "namespace": "import * as X from './x.js';\nconst kept = 1;\n",
        "double quotes": 'import { X } from "./x.js";\nconst kept = 1;\n',
        # websocket-tile-layer.js imports over several lines.
        "multi-line": "import {\n  a,\n  b,\n} from './x.js';\nconst kept = 1;\n",
        # No semicolon: the regex used to eat up to the next one.
        "no semicolon": "import X from './x.js'\nconst kept = 1;\nlet also = 2;\n",
    }
    for name, source in cases.items():
        out = embed.process_js_file(source)
        check(f"import {name} stripped", "import" not in out, out)
        check(f"import {name} keeps the code after it", "kept" in out, out)
        if name == "no semicolon":
            check("no semicolon keeps every later statement", "also" in out, out)


def kept_lookalikes():
    """Text that only mentions an import is not code, and stays."""
    comment = "// main.js and the tests import it from here\nconst kept = 1;\n"
    check("comment kept", "import it from here" in embed.process_js_file(comment))

    text = "const s = 'import a; b';\nconst kept = 1;\n"
    check("string kept", "import a; b" in embed.process_js_file(text))


def exported_names_forwarded():
    """Every export form the sources use reaches the outer scope."""
    cases = {
        "function": ("export function make() {}\n", "make"),
        "async function": ("export async function renderTile() {}\n", "renderTile"),
        "class": ("export class Widget {}\n", "Widget"),
        "const": ("export const kMargin = 4;\n", "kMargin"),
        "block": (
            "function floorClampZoom() {}\nexport { floorClampZoom };\n",
            "floorClampZoom",
        ),
        "renamed": ("function inner() {}\nexport { inner as outer };\n", "outer"),
    }
    for name, (source, exported) in cases.items():
        out = embed.process_js_file(source)
        check(f"export {name} stripped", "export" not in out, out)
        check(
            f"export {name} forwards {exported}",
            f"var {exported}" in out or f", {exported}" in out,
            out,
        )


def unhandled_forms_fail_the_build():
    """What the patterns miss has to stop the build, not the browser."""
    for name, source in {
        "export default": "export default 1;\n",
        "export let": "export let counter = 0;\n",
        # Left in place on purpose: it cannot be resolved in one scope.
        "dynamic import": "import('./x.js')\n",
    }.items():
        processed = embed.process_js_file(source)
        check_raises(
            f"{name} fails the build",
            lambda text=processed: embed.check_no_module_syntax(text),
        )


def raw_literal_hazards_fail_the_build():
    """Content a raw string literal cannot carry verbatim."""
    for name, text in {
        "delimiter": f'const s = `){embed.JS_DELIMITER}"`;',
        "carriage return": "const s = 1;\r\n",
        "NUL": "const s = '\0';",
    }.items():
        check_raises(
            f"{name} fails the build",
            lambda t=text: embed.check_embeddable(t, embed.JS_DELIMITER, "test input"),
        )
    embed.check_embeddable("const s = 1;\n", embed.JS_DELIMITER, "test input")


def main():
    stripped_imports()
    kept_lookalikes()
    exported_names_forwarded()
    unhandled_forms_fail_the_build()
    raw_literal_hazards_fail_the_build()

    if failures:
        print("\n".join(failures), file=sys.stderr)
        return 1
    print("embed_report_assets.py behaves as pinned")
    return 0


if __name__ == "__main__":
    sys.exit(main())
