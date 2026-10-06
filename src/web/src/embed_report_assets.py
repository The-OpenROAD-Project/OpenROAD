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
import sys


def process_js_file(content):
    """Process a single JS file for concatenation into a shared scope."""
    # Remove import statements, including ones wrapped over several lines.
    # `[^;]*` spans newlines but stops at the first ';', so a statement is
    # consumed whole and no real code can be swallowed.  A single-line-only
    # pattern left the tail of a wrapped import behind, which is a syntax error
    # for the whole concatenated bundle.
    aliases = import_aliases(content)
    content = re.sub(r"^import\s[^;]*;[ \t]*$", "", content, flags=re.MULTILINE)
    # An aliased import loses its local name with the import; take it from
    # the global the exporting module leaves behind.
    if aliases:
        content = (
            "\n".join(f"const {local} = {name};" for name, local in aliases)
            + "\n"
            + content
        )

    # Find exported names and strip the export keyword.
    # Two patterns:
    #   1. export function/class/const Name ...
    #   2. export { InternalA as ExportedA, InternalB, ... }
    exported_names = []

    # Pattern 1: export [async] function/class/const Name
    def capture_export_decl(m):
        keyword = m.group(1)  # [async] function, class, or const
        name = m.group(2)
        exported_names.append(name)
        return keyword + " " + name

    content = re.sub(
        r"^export\s+((?:async\s+)?function|class|const)\s+(\w+)",
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


def import_aliases(content):
    """(name, local) for each `name as local` in a named import."""
    pairs = []
    for m in re.finditer(r"^import\s*\{([^}]*)\}[^;]*;", content, flags=re.MULTILINE):
        for item in m.group(1).split(","):
            parts = item.split()
            if len(parts) == 3 and parts[1] == "as":
                pairs.append((parts[0], parts[2]))
    for name, _ in pairs:
        if re.search(
            r"^(?:export\s+)?(?:async\s+)?(?:function|class|const|let|var)\s+"
            + name
            + r"\b",
            content,
            flags=re.MULTILINE,
        ):
            sys.exit(f"embed_report_assets: '{name}' is both imported and declared")
    return pairs


def local_imports(content):
    """Basenames of the sibling modules a JS file imports ('./x.js')."""
    return re.findall(
        r"^import\s(?:[^;]*?from\s+)?['\"]\./([\w.-]+\.js)['\"]",
        content,
        flags=re.MULTILINE,
    )


def dependency_order(sources):
    """`sources` ([(path, content)]) with each file after the files it imports,
    else in the given order: a file's top-level code runs as it is reached."""
    by_name = {path.split("/")[-1]: (path, content) for path, content in sources}
    done, visiting, ordered = set(), [], []

    def visit(name):
        if name in done:
            return
        if name in visiting:
            cycle = visiting[visiting.index(name) :] + [name]
            sys.exit("embed_report_assets: import cycle " + " -> ".join(cycle))
        visiting.append(name)
        for dep in local_imports(by_name[name][1]):
            if dep not in by_name:
                sys.exit(
                    f"embed_report_assets: {name} imports {dep}, "
                    "which is not in --js"
                )
            visit(dep)
        visiting.pop()
        done.add(name)
        ordered.append(by_name[name])

    for path, _ in sources:
        visit(path.split("/")[-1])
    return ordered


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", "-o", required=True)
    parser.add_argument("--css", required=True, help="style.css path")
    parser.add_argument(
        "--js",
        nargs="+",
        required=True,
        help="JS files; each is emitted after the files it imports",
    )
    args = parser.parse_args()

    # Read and process JS files.
    sources = []
    for path in args.js:
        with open(path, encoding="utf-8") as f:
            sources.append((path, f.read()))
    js_parts = []
    for path, content in dependency_order(sources):
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
