# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

# We need this rule as long as we support Tcl 8

"""
Unzip a zip file into a directory.

Since this creates a directory, we need to have it as separate rule
and can't just have a genrule().
"""

def _extract_zip_impl(ctx):
    out_dir = ctx.actions.declare_directory(ctx.attr.name)
    ctx.actions.run(
        inputs = [ctx.file.src],
        outputs = [out_dir],
        executable = ctx.executable._zipper,
        arguments = ["x", ctx.file.src.path, "-d", out_dir.path],
    )
    return [DefaultInfo(
        files = depset([out_dir]),
        runfiles = ctx.runfiles(files = [out_dir]),
    )]

extract_zip = rule(
    implementation = _extract_zip_impl,
    attrs = {
        "src": attr.label(allow_single_file = [".zip", ".jar"]),
        "_zipper": attr.label(
            default = Label("@bazel_tools//tools/zip:zipper"),
            cfg = "exec",
            executable = True,
        ),
    },
)
