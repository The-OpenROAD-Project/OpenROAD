# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2020-2026, The OpenROAD Authors
"""Build rule for generating C or C++ sources with Bison.

Runs the Bazel-built bison from the `bison` BCR module. That bison has no
data or m4 paths compiled in, so the action passes them through
BISON_PKGDATADIR and M4, as @bison//:bison.bzl does.
"""

def _pkgdatadir(files):
    """Derives BISON_PKGDATADIR from @bison//:bison_pkgdatadir.

    Mirrors _resolve_pkgdatadir in @bison//:bison.bzl: the label resolves
    either to one tree artifact, or to data files that share a `data`
    path component.
    """
    if len(files) == 1 and files[0].is_directory:
        return files[0].path
    for f in files:
        index = f.path.find("/data/")
        if index >= 0:
            return f.path[:index + 5]
        if f.path.startswith("data/"):
            return "data"
    fail("Could not determine BISON_PKGDATADIR: no 'data' directory in %s" % files)

def _tool_files(target):
    """A target's own files plus its default runfiles, for `tools`."""
    info = target[DefaultInfo]
    transitive = [info.files]
    if info.default_runfiles:
        transitive.append(info.default_runfiles.files)
    return depset(transitive = transitive)

def _genyacc_impl(ctx):
    """Implementation for genyacc rule."""

    # Argument list
    args = ctx.actions.args()
    args.add(ctx.outputs.header_out.path, format = "--defines=%s")
    args.add(ctx.outputs.source_out.path, format = "--output-file=%s")
    if ctx.attr.prefix:
        args.add(ctx.attr.prefix, format = "--name-prefix=%s")
    args.add_all([ctx.expand_location(opt) for opt in ctx.attr.extra_options])
    args.add(ctx.file.src.path)

    # Output files
    outputs = ctx.outputs.extra_outs + [
        ctx.outputs.header_out,
        ctx.outputs.source_out,
    ]

    ctx.actions.run(
        executable = ctx.executable._bison,
        env = {
            "BISON_PKGDATADIR": _pkgdatadir(ctx.files._bison_pkgdatadir),
            "M4": ctx.file._m4.path,
        },
        arguments = [args],
        inputs = ctx.files.src,
        tools = depset(transitive = [
            _tool_files(ctx.attr._bison),
            _tool_files(ctx.attr._m4),
            _tool_files(ctx.attr._bison_pkgdatadir),
        ]),
        outputs = outputs,
        mnemonic = "Yacc",
        progress_message = "Generating %s and %s from %s" %
                           (
                               ctx.outputs.source_out.short_path,
                               ctx.outputs.header_out.short_path,
                               ctx.file.src.short_path,
                           ),
    )

genyacc = rule(
    implementation = _genyacc_impl,
    doc = "Generate C/C++-language sources from a Yacc file using Bison.",
    attrs = {
        "extra_options": attr.string_list(
            doc = "A list of extra options to pass to Bison.  These are " +
                  "subject to $(location ...) expansion.",
        ),
        "extra_outs": attr.output_list(
            doc = "A list of extra generated output files.",
        ),
        "header_out": attr.output(
            mandatory = True,
            doc = "The generated 'defines' header file",
        ),
        "prefix": attr.string(
            doc = "External symbol prefix for Bison. This string is " +
                  "passed to bison as the -p option, causing the resulting C " +
                  "file to define external functions named 'prefix'parse, " +
                  "'prefix'lex, etc. instead of yyparse, yylex, etc.",
        ),
        "source_out": attr.output(
            mandatory = True,
            doc = "The generated source file",
        ),
        "src": attr.label(
            mandatory = True,
            allow_single_file = [".y", ".yy", ".yc", ".ypp", ".yxx"],
            doc = "The .y, .yy, or .yc source file for this rule",
        ),
        "_bison": attr.label(
            default = Label("@bison//:bison"),
            cfg = "exec",
            executable = True,
        ),
        "_bison_pkgdatadir": attr.label(
            default = Label("@bison//:bison_pkgdatadir"),
            allow_files = True,
        ),
        "_m4": attr.label(
            default = Label("@bison//:m4"),
            cfg = "exec",
            allow_single_file = True,
        ),
    },
)
