# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Bazel rule to generate man pages (cat/ and html/) from module READMEs.

Runs the existing docs Makefile to produce cat/ and html/ outputs.
Requires pandoc, nroff, and python3 (>= 3.10) on the system.
"""

def _man_pages_impl(ctx):
    cat_dir = ctx.actions.declare_directory("cat")
    html_dir = ctx.actions.declare_directory("html")

    command = """
set -euo pipefail

# Ensure homebrew tools (pandoc, python3 >= 3.10) are available on macOS
if [ -d /opt/homebrew/bin ]; then
    export PATH="/opt/homebrew/bin:$PATH"
elif [ -d /usr/local/bin ]; then
    export PATH="/usr/local/bin:$PATH"
fi

cd docs

# Symlink module READMEs into md/man2/
bash src/scripts/link_readmes.sh 2>/dev/null || true

# Run preprocessing
python3 src/scripts/md_roff_compat.py

# Generate cat and html pages via Makefile
make -f Makefile cat web

cd ..

# Copy outputs to declared Bazel directories
cp -r docs/cat/* {cat_out}/
cp -r docs/html/* {html_out}/
""".format(
        cat_out = cat_dir.path,
        html_out = html_dir.path,
    )

    ctx.actions.run_shell(
        outputs = [cat_dir, html_dir],
        inputs = ctx.files.docs_srcs + ctx.files.scripts + ctx.files.readmes + ctx.files.messages,
        command = command,
        mnemonic = "ManPages",
        progress_message = "Generating man pages (cat + html)",
        use_default_shell_env = True,
        execution_requirements = {"no-sandbox": "1"},
    )

    return [DefaultInfo(
        files = depset([cat_dir, html_dir]),
        runfiles = ctx.runfiles(files = [cat_dir, html_dir]),
    )]

man_pages = rule(
    implementation = _man_pages_impl,
    attrs = {
        "docs_srcs": attr.label_list(
            doc = "All source files under docs/ needed by the Makefile.",
            allow_files = True,
        ),
        "messages": attr.label_list(
            doc = "Module messages.txt files needed for man3 page generation.",
            allow_files = [".txt"],
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
)
