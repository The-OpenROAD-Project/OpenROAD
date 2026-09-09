# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Bazel rule that invokes docs/Makefile to produce cat/ and html/ man pages.

The output filenames aren't known at analysis time (they depend on which
modules exist under src/*/), so outputs are declared as TreeArtifacts and
the real work is delegated to the `bazel-manpages` Makefile target.

Host requirements: nroff (groff), col (bsdextrautils). Python comes from the
Bazel toolchain and pandoc from a pinned wheel, not the host.
"""

_PY_TOOLCHAIN_TYPE = "@rules_python//python:toolchain_type"

def _man_pages_resource_set(_os, _num_inputs):
    # The 'cat web' make below fans out with -j$(nproc), so this action uses
    # the whole host. Reserve all local CPUs to keep Bazel from co-scheduling
    # other heavy actions alongside it and oversubscribing the machine. Bazel
    # clamps the request to the cores actually available, so this is safe on
    # small CI hosts too.
    return {"cpu": 512.0}

def _man_pages_impl(ctx):
    cat_dir = ctx.actions.declare_directory("cat")
    html_dir = ctx.actions.declare_directory("html")

    # The Makefile's preprocess step runs md_roff_compat.py, which needs
    # Python >= 3.7 ('from __future__ import annotations'). Take the interpreter
    # from the toolchain instead of leaving the Makefile on the host's python3:
    # RHEL 8 ships 3.6 as /usr/bin/python3, and the action's environment is not
    # the interactive shell's, so a newer python3 earlier on the user's PATH
    # does not necessarily reach it either.
    py3_runtime = ctx.toolchains[_PY_TOOLCHAIN_TYPE].py3_runtime
    interpreter_files = []
    if py3_runtime.interpreter:
        # An in-build interpreter: its path is execroot-relative, and make runs
        # with -C docs, so anchor it to the execroot the action starts in.
        python = "$PWD/" + py3_runtime.interpreter.path
        interpreter_files = [py3_runtime.files]
    else:
        # A platform runtime instead: an absolute path on the host.
        python = py3_runtime.interpreter_path

    # Same reasoning for pandoc, which renders every man and html page: the
    # host's version decides the output and RHEL 8 has no pandoc package at
    # all. The pypandoc-binary wheel (pinned in bazel/requirements.in) ships a
    # statically linked pandoc, so take the binary out of the wheel rather than
    # off PATH. The wheel's other files are the Python bindings, which the doc
    # build does not use, so only the binary becomes an action input.
    pandoc = None
    for f in ctx.files.pandoc:
        if f.basename == "pandoc":
            pandoc = f
            break
    if not pandoc:
        fail("no 'pandoc' binary among the files of %s" % ctx.attr.pandoc.label)

    command = """
set -euo pipefail
CAT_OUT="$PWD/{cat_out}"
HTML_OUT="$PWD/{html_out}"
# The messages.txt files that man3 is built from are generated, so they live
# under bazel-out rather than in the source tree the execroot symlinks in.
# md_roff_compat.py looks for <root>/src/<module>/messages.txt and
# <root>/messages.txt, which is exactly the bin dir's layout.
export MESSAGES_ROOT_DIR="$PWD/{bin_dir}"
# use_default_shell_env passes the client's environment through (groff and col
# are found on PATH). Drop the two variables that would redirect the toolchain
# interpreter at a host installation's stdlib.
unset PYTHONHOME PYTHONPATH
# Two phases: 'preprocess' (serial) generates the md/man*/*.md sources, then
# 'cat web' fan out pandoc/nroff in parallel. They cannot share one -j make
# invocation: cat/web read the md files preprocess produces, and a parallel
# build has no dependency edge forcing preprocess to finish first. Running
# 'cat web' as a second invocation also re-parses the Makefile so its
# $(wildcard md/man*/*.md) picks up the freshly generated sources.
# nproc is GNU coreutils (absent on stock macOS); fall back to sysctl, then 4.
JOBS="$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"
# -s (silent) suppresses make's per-recipe echo. There are ~3400 man pages and
# four recipes each, so echoing them emits megabytes of stdout, past Bazel's
# --experimental_ui_max_stdouterr_bytes, which then drops the whole log --
# including the pandoc/groff diagnostics that are worth seeing. Those tools
# name the file they failed on, so the echoed command adds nothing.
make -s --no-print-directory -C docs -f Makefile preprocess \\
    PYTHON="{python}" CAT_ROOT_DIR="$CAT_OUT" HTML_ROOT_DIR="$HTML_OUT"
make -s --no-print-directory -j"$JOBS" -C docs -f Makefile cat web \\
    PANDOC="$PWD/{pandoc}" CAT_ROOT_DIR="$CAT_OUT" HTML_ROOT_DIR="$HTML_OUT"
""".format(
        bin_dir = ctx.bin_dir.path,
        cat_out = cat_dir.path,
        html_out = html_dir.path,
        pandoc = pandoc.path,
        python = python,
    )

    ctx.actions.run_shell(
        resource_set = _man_pages_resource_set,
        outputs = [cat_dir, html_dir],
        inputs = depset(
            ctx.files.docs_srcs + ctx.files.scripts + ctx.files.readmes +
            ctx.files.messages + [pandoc],
            transitive = interpreter_files,
        ),
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
        "pandoc": attr.label(
            doc = "Wheel files holding the pandoc binary the Makefile runs.",
            default = "@openroad-pip//pypandoc_binary:extracted_whl_files",
            allow_files = True,
            # pandoc runs during the build, so pick the wheel for the platform
            # that executes the action, not the one being built for.
            cfg = "exec",
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
    toolchains = [_PY_TOOLCHAIN_TYPE],
)
