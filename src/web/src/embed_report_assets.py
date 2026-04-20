#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Embed JS/CSS files as C++ raw string literals for the standalone
# timing report HTML.  Produces a .cpp file with const char* constants.
#
# Each JS file is wrapped in an IIFE to isolate file-private const/let
# declarations.  Exported symbols are forwarded to outer-scope vars.
#
# Vendored third-party assets (leaflet, golden-layout) are also embedded
# so the generated report is self-contained and renders with no CDN
# connection — see issue #10201. The golden-layout ESM bundle is
# transformed so its exports land on `window.GoldenLayout` and
# `window.LayoutConfig`, making them visible to the (stripped-imports)
# application script.

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


def transform_golden_layout_esm(content, wanted=("GoldenLayout", "LayoutConfig")):
    """Convert the esm.sh golden-layout.mjs bundle into an IIFE that
    assigns the requested exports onto `window`. The bundle uses `var`
    declarations and a single trailing `export { <internal> as <exported>, ... };`
    statement (no import.meta / top-level await), so wrapping in an IIFE
    is safe."""
    # Strip the sourceMappingURL comment and the esm.sh header banner —
    # the banner's literal "esm.sh" string otherwise shows up in a
    # network-scan of the emitted HTML and misleads offline audits.
    content = re.sub(r"//#\s*sourceMappingURL=.*$", "", content, flags=re.MULTILINE)
    content = re.sub(r"^\s*/\*\s*esm\.sh[^*]*\*/\s*", "", content)
    # Locate and strip the trailing export block.
    m = re.search(r"export\s*\{([^}]+)\}\s*;?\s*$", content)
    if not m:
        raise RuntimeError("golden-layout bundle missing trailing export {...}")
    export_list = m.group(1)
    body = content[: m.start()]
    alias = {}  # exported name -> internal name
    for item in export_list.split(","):
        item = item.strip()
        if not item:
            continue
        parts = re.split(r"\s+as\s+", item)
        if len(parts) == 2:
            alias[parts[1].strip()] = parts[0].strip()
        else:
            alias[parts[0]] = parts[0]
    missing = [n for n in wanted if n not in alias]
    if missing:
        raise RuntimeError(
            "golden-layout bundle does not export: " + ", ".join(missing)
        )
    assignments = "\n".join(
        "  window.{e} = {i};".format(e=e, i=alias[e]) for e in wanted
    )
    return (
        "(function() {\n" + body.strip() + "\n" + assignments + "\n})();\n"
    )


def read(path):
    with open(path, encoding="utf-8") as f:
        return f.read()


def raw_string_literal(marker, value):
    """Emit a C++ raw string literal using a marker that cannot appear in
    the content. The bundled JS/CSS never uses our markers."""
    return 'R"' + marker + "(\n" + value + "\n)" + marker + '";'


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", "-o", required=True)
    parser.add_argument("--css", required=True, help="style.css path")
    parser.add_argument(
        "--js", nargs="+", required=True, help="JS files in dependency order"
    )
    # Vendored third-party assets — inlined verbatim so the report renders
    # without any CDN connection.
    parser.add_argument("--leaflet-js", required=True)
    parser.add_argument("--leaflet-css", required=True)
    parser.add_argument("--golden-layout-esm", required=True)
    parser.add_argument("--golden-layout-base-css", required=True)
    parser.add_argument("--golden-layout-dark-css", required=True)
    args = parser.parse_args()

    # Read and process application JS files.
    js_parts = []
    for path in args.js:
        content = read(path)
        js_parts.append(f'// ── {path.split("/")[-1]} ──')
        js_parts.append(process_js_file(content))
    combined_js = "\n".join(js_parts)

    css_content = read(args.css)
    leaflet_js = read(args.leaflet_js)
    leaflet_css = read(args.leaflet_css)
    golden_layout_js = transform_golden_layout_esm(read(args.golden_layout_esm))
    golden_layout_base_css = read(args.golden_layout_base_css)
    golden_layout_dark_css = read(args.golden_layout_dark_css)

    with open(args.output, "w", encoding="utf-8") as out:
        out.write("// Auto-generated — do not edit.\n")
        out.write("#include <string_view>\n")
        out.write("namespace web {\n")
        pairs = [
            ("kReportCSS", "__CSS__", css_content),
            ("kReportJS", "__JS__", combined_js),
            ("kLeafletJS", "__LFJS__", leaflet_js),
            ("kLeafletCSS", "__LFCSS__", leaflet_css),
            ("kGoldenLayoutJS", "__GLJS__", golden_layout_js),
            ("kGoldenLayoutBaseCSS", "__GLBCSS__", golden_layout_base_css),
            ("kGoldenLayoutDarkCSS", "__GLDCSS__", golden_layout_dark_css),
        ]
        for name, marker, value in pairs:
            out.write("extern const std::string_view " + name + " = ")
            out.write(raw_string_literal(marker, value))
            out.write("\n\n")
        out.write("}  // namespace web\n")


if __name__ == "__main__":
    main()
