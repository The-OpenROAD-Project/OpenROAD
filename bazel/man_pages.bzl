# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Bazel rule that produces cat/ and html/ man pages.

The output filenames aren't known at analysis time (they depend on which
modules exist under src/*/), so outputs are declared as TreeArtifacts.

Host requirements: pandoc, nroff (groff), and col (bsdextrautils).
"""

def _shquote(value):
    return "'" + value.replace("'", "'\\''") + "'"

def _man_pages_impl(ctx):
    cat_dir = ctx.actions.declare_directory("cat")
    html_dir = ctx.actions.declare_directory("html")

    python = ctx.toolchains["@rules_python//python:toolchain_type"].py3_runtime.interpreter
    work_dir = ctx.actions.declare_directory(ctx.label.name + "_work")

    copy_commands = []
    for src in ctx.files.docs_srcs + ctx.files.scripts:
        copy_commands.append("mkdir -p \"$(dirname \"$WORK/{dst}\")\" && cp -f {src} \"$WORK/{dst}\"".format(
            dst = src.short_path,
            src = _shquote(src.path),
        ))
    for src in ctx.files.messages:
        dst = src.short_path
        copy_commands.append("mkdir -p \"$(dirname \"$WORK/{dst}\")\" && cp -f {src} \"$WORK/{dst}\"".format(
            dst = dst,
            src = _shquote(src.path),
        ))
    for src in ctx.files.readmes:
        module = src.short_path.split("/")[-2]
        copy_commands.append("mkdir -p \"$WORK/docs/md/man2\" && cp -f {src} \"$WORK/docs/md/man2/{module}.md\"".format(
            module = module,
            src = _shquote(src.path),
        ))

    command = """
set -euo pipefail
EXEC_ROOT="$PWD"
WORK="$EXEC_ROOT"/{work_dir}
CAT_OUT="$EXEC_ROOT"/{cat_out}
HTML_OUT="$EXEC_ROOT"/{html_out}
PYTHON={python}
PANDOC={pandoc}
NROFF={nroff}
COL={col}

rm -rf "$WORK" "$CAT_OUT" "$HTML_OUT"
mkdir -p "$WORK/docs/md/man1" "$WORK/docs/md/man2" "$WORK/docs/md/man3"
mkdir -p "$CAT_OUT" "$HTML_OUT"

{copy_commands}

cd "$WORK/docs"
"$PYTHON" src/scripts/md_roff_compat.py

MAN_ROOT="$PWD/man"
mkdir -p "$MAN_ROOT"/man1 "$MAN_ROOT"/man2 "$MAN_ROOT"/man3
mkdir -p "$HTML_OUT"/html1 "$HTML_OUT"/html2 "$HTML_OUT"/html3
mkdir -p "$CAT_OUT"/cat1 "$CAT_OUT"/cat2 "$CAT_OUT"/cat3

is_module_readme() {{
    case "$1" in
        ant|cts|dbSta|dft|dpl|drt|dst|fin|gpl|grt|gui|ifp|mpl|odb|pad|par|pdn|ppl|psm|rcx|rmp|rsz|sta|stt|tap|upf|utl)
            return 0
            ;;
        *)
            return 1
            ;;
    esac
}}

convert_manpage() {{
    src="$1"
    section="$2"
    html_subdir="$3"
    cat_subdir="$4"
    base="$(basename "$src" .md)"
    man_file="$MAN_ROOT/man$section/$base.$section"
    html_file="$HTML_OUT/$html_subdir/$base.html"
    cat_file="$CAT_OUT/$cat_subdir/$base.$section"

    "$PANDOC" -s -t man "$src" -o "$man_file" --quiet
    "$PANDOC" -s -f man -t html "$man_file" -o "$html_file" --quiet
    "$NROFF" -man "$man_file" | "$COL" -b > "$cat_file"
}}

shopt -s nullglob
for src in md/man1/*.md; do
    convert_manpage "$src" 1 html1 cat1
done

for src in md/man2/*.md; do
    base="$(basename "$src" .md)"
    if is_module_readme "$base"; then
        continue
    fi
    convert_manpage "$src" 2 html2 cat2
done

for src in md/man3/*.md; do
    convert_manpage "$src" 3 html3 cat3
done
""".format(
        cat_out = _shquote(cat_dir.path),
        col = _shquote(ctx.attr.col),
        copy_commands = "\n".join(copy_commands),
        html_out = _shquote(html_dir.path),
        nroff = _shquote(ctx.attr.nroff),
        pandoc = _shquote(ctx.attr.pandoc),
        python = _shquote(python.path),
        work_dir = _shquote(work_dir.path),
    )

    ctx.actions.run_shell(
        outputs = [work_dir, cat_dir, html_dir],
        inputs = ctx.files.docs_srcs + ctx.files.scripts + ctx.files.readmes + ctx.files.messages,
        command = command,
        mnemonic = "ManPages",
        progress_message = "Generating man pages (cat + html)",
        tools = [python],
    )

    return [DefaultInfo(
        files = depset([cat_dir, html_dir]),
        runfiles = ctx.runfiles(files = [cat_dir, html_dir]),
    )]

man_pages = rule(
    implementation = _man_pages_impl,
    attrs = {
        "docs_srcs": attr.label_list(
            doc = "All source files under docs/ needed to generate man pages.",
            allow_files = True,
        ),
        "col": attr.string(
            default = "col",
            doc = "Executable used to remove nroff backspace formatting.",
        ),
        "messages": attr.label_list(
            doc = "Module messages.txt files needed for man3 page generation.",
            allow_files = [".txt"],
        ),
        "nroff": attr.string(
            default = "nroff",
            doc = "Executable used to render cat man pages.",
        ),
        "pandoc": attr.string(
            default = "pandoc",
            doc = "Executable used to convert Markdown and roff man pages.",
        ),
        "readmes": attr.label_list(
            doc = "Module README.md files (src/*/README.md).",
            allow_files = [".md"],
        ),
        "scripts": attr.label_list(
            doc = "Python/shell scripts for man page generation.",
            allow_files = True,
        ),
    },
    toolchains = ["@rules_python//python:toolchain_type"],
)
