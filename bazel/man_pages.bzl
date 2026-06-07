# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Bazel rule that invokes docs/Makefile to produce cat/ and html/ man pages.

The output filenames aren't known at analysis time (they depend on which
modules exist under src/*/), so outputs are declared as TreeArtifacts and
the real work is delegated to the `bazel-manpages` Makefile target.

Host requirements: pandoc, nroff (groff), col (bsdextrautils), python3>=3.10.
"""

def _man_pages_impl(ctx):
    cat_dir = ctx.actions.declare_directory("cat")
    html_dir = ctx.actions.declare_directory("html")

    command = """
set -euo pipefail
CAT_OUT="$PWD/{cat_out}"
HTML_OUT="$PWD/{html_out}"
make --no-print-directory -C docs -f Makefile bazel-manpages \\
    CAT_ROOT_DIR="$CAT_OUT" HTML_ROOT_DIR="$HTML_OUT"
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
