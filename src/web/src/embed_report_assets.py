#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Embed JS/CSS files as C++ raw string literals for the standalone
# timing report HTML.  Produces a .cpp file with const char* constants.
#
# Each JS file is wrapped in an IIFE to isolate file-private const/let
# declarations.  Exported symbols are forwarded to outer-scope vars.

import argparse
import re


def process_js_file(content):
    """Process a single JS file for concatenation into a shared scope."""
    # Remove import lines.
    content = re.sub(r"^import\s+.*;\s*$", "", content, flags=re.MULTILINE)

    # Find exported names and strip the export keyword.
    # Two patterns:
    #   1. export function/class/const Name ...
    #   2. export { InternalA as ExportedA, InternalB, ... }
    exported_names = []

    # Pattern 1: export function/class/const Name
    def capture_export_decl(m):
        keyword = m.group(1)  # function, class, or const
        name = m.group(2)
        exported_names.append(name)
        return keyword + " " + name

    content = re.sub(
        r"^export\s+(function|class|const)\s+(\w+)",
        capture_export_decl,
        content,
        flags=re.MULTILINE,
    )

    # Pattern 2: export { InternalA as ExportedA, InternalB, ... }
    # Collects (internal_name, exported_name) pairs; removes the line;
    # will add alias assignments after the IIFE.
    renamed_exports = []  # (internal, exported)

    def capture_export_block(m):
        for item in m.group(1).split(","):
            item = item.strip()
            if not item:
                continue
            if " as " in item:
                internal, exported = item.split(" as ", 1)
                renamed_exports.append((internal.strip(), exported.strip()))
            else:
                renamed_exports.append((item, item))
        return ""  # remove the export block

    content = re.sub(r"export\s*\{([^}]+)\}", capture_export_block, content)

    content = content.strip()
    if not content:
        return ""

    # Merge both export lists into a unified set of outer-scope names.
    all_exported = list(exported_names)
    for _, exported in renamed_exports:
        if exported not in all_exported:
            all_exported.append(exported)

    if not all_exported:
        # No exports — wrap in a bare block for const/let isolation.
        return "{\n" + content + "\n}"

    # Wrap in an IIFE.  Exported names get outer-scope `var` declarations.
    lines = []
    lines.append("var " + ", ".join(all_exported) + ";")
    # IIFE returns an object with the exported names.
    # For pattern-1 exports, the name is the same inside and out.
    # For pattern-2 exports, we map internal -> exported.
    return_pairs = []
    for name in exported_names:
        return_pairs.append(name + ": " + name)
    for internal, exported in renamed_exports:
        return_pairs.append(exported + ": " + internal)
    exports_obj = "{ " + ", ".join(return_pairs) + " }"
    lines.append("var __e = (function() {")
    lines.append(content)
    lines.append("return " + exports_obj + ";")
    lines.append("})();")
    # Assign from returned object to outer vars.
    for name in all_exported:
        lines.append(name + " = __e." + name + ";")
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", "-o", required=True)
    parser.add_argument("--css", required=True, help="style.css path")
    parser.add_argument(
        "--js", nargs="+", required=True, help="JS files in dependency order"
    )
    args = parser.parse_args()

    # Read and process JS files.
    js_parts = []
    for path in args.js:
        with open(path, encoding="utf-8") as f:
            content = f.read()
        js_parts.append(f'// ── {path.split("/")[-1]} ──')
        js_parts.append(process_js_file(content))
    combined_js = "\n".join(js_parts)

    # Read CSS.
    with open(args.css, encoding="utf-8") as f:
        css_content = f.read()

    with open(args.output, "w", encoding="utf-8") as out:
        out.write("// Auto-generated — do not edit.\n")
        out.write("#include <string_view>\n")
        out.write("namespace web {\n")
        out.write('extern const std::string_view kReportCSS = R"__CSS__(\n')
        out.write(css_content)
        out.write(')__CSS__";\n\n')
        out.write('extern const std::string_view kReportJS = R"__JS__(\n')
        out.write(combined_js)
        out.write(')__JS__";\n')
        out.write("}  // namespace web\n")


if __name__ == "__main__":
    main()
