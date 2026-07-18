# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Hermetic Bazel rule that generates cat/ and HTML/ man pages.

All tools (pandoc, Python interpreter) and all inputs (README files,
messages.txt files, man1 source markdown, Python scripts) are declared
as explicit Bazel dependencies.  No host PATH look-ups, no make, no
nroff — pandoc --to=plain replaces nroff+col for cat pages.

Output filenames are not known at analysis time (they depend on how
many Tcl commands each module exposes), so outputs are declared as
TreeArtifacts.
"""

def _man_pages_impl(ctx):
    cat_dir = ctx.actions.declare_directory("cat")
    html_dir = ctx.actions.declare_directory("html")

    pandoc = ctx.file._pandoc
    impl = ctx.executable._manpages_impl

    args = ctx.actions.args()
    args.add("--pandoc", pandoc)
    args.add("--cat-out", cat_dir.path)
    args.add("--html-out", html_dir.path)

    for f in ctx.files.scripts:
        if f.basename.endswith(".py"):
            args.add("--script", f)

    for readme in ctx.files.readmes:
        # Derive module name: src/ant/README.md  →  ant
        parts = readme.short_path.split("/")
        module = parts[-2] if len(parts) >= 2 else readme.basename
        args.add("--readme", "{}:{}".format(module, readme.path))

    for msg in ctx.files.messages:
        # Derive module name: src/ant/messages.txt  →  ant
        parts = msg.short_path.split("/")
        module = parts[-2] if len(parts) >= 2 else msg.basename
        args.add("--messages", "{}:{}".format(module, msg.path))

    for f in ctx.files.docs_srcs:
        if "/man1/" in f.path and f.basename.endswith(".md"):
            args.add("--man1-src", f)

    all_inputs = depset(
        ctx.files.docs_srcs +
        ctx.files.scripts +
        ctx.files.readmes +
        ctx.files.messages +
        [pandoc],
        transitive = [ctx.attr._manpages_impl[DefaultInfo].default_runfiles.files],
    )

    ctx.actions.run(
        outputs = [cat_dir, html_dir],
        inputs = all_inputs,
        executable = impl,
        arguments = [args],
        mnemonic = "ManPages",
        progress_message = "Generating man pages (cat + html)",
    )

    return [DefaultInfo(
        files = depset([cat_dir, html_dir]),
        runfiles = ctx.runfiles(files = [cat_dir, html_dir]),
    )]

man_pages = rule(
    implementation = _man_pages_impl,
    attrs = {
        "docs_srcs": attr.label_list(
            doc = "Source .md files under docs/md/ needed for man page generation.",
            allow_files = True,
        ),
        "messages": attr.label_list(
            doc = "Module messages.txt files for man3 page generation.",
            allow_files = [".txt"],
        ),
        "readmes": attr.label_list(
            doc = "Module README.md files (src/*/README.md).",
            allow_files = [".md"],
        ),
        "scripts": attr.label_list(
            doc = "Python scripts for man page generation.",
            allow_files = True,
        ),
        "_manpages_impl": attr.label(
            default = "//bazel:manpages_impl",
            executable = True,
            cfg = "exec",
        ),
        "_pandoc": attr.label(
            default = "//bazel:pandoc",
            allow_single_file = True,
            cfg = "exec",
        ),
    },
)
