# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2020-2026, The OpenROAD Authors

"""Build rule for generating C or C++ sources with Bison.
"""

def correct_bison_env_for_action(env, bison):
    """Modify the Bison environment variables to work in an action that doesn't a have built bison runfiles directory.

    The `bison_toolchain.bison_env` parameter assumes that Bison will provided via an executable attribute
    and thus have built runfiles available to it. This is not the case for this action and any other actions
    trying to use bison as a tool via the toolchain. This function transforms existing environment variables
    to support running Bison as desired.

    Args:
        env (dict): The existing bison environment variables
        bison (File): The Bison executable

    Returns:
        Dict: Environment variables required for running Bison.
    """
    bison_env = dict(env)

    # Convert the environment variables to non-runfiles forms
    bison_runfiles_dir = "{}.runfiles/{}".format(
        bison.path,
        bison.owner.workspace_name,
    )

    bison_env["BISON_PKGDATADIR"] = bison_env["BISON_PKGDATADIR"].replace(
        bison_runfiles_dir,
        "external/{}".format(bison.owner.workspace_name),
    )
    bison_env["M4"] = bison_env["M4"].replace(
        bison_runfiles_dir,
        "{}/external/{}".format(bison.root.path, bison.owner.workspace_name),
    )

    return bison_env

def _genyacc_impl(ctx):
    """Implementation for genyacc rule."""

    bison_toolchain = ctx.toolchains["@rules_bison//bison:toolchain_type"].bison_toolchain

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
        executable = bison_toolchain.bison_tool.executable,
        env = correct_bison_env_for_action(
            env = bison_toolchain.bison_env,
            bison = bison_toolchain.bison_tool.executable,
        ),
        arguments = [args],
        inputs = ctx.files.src,
        tools = [bison_toolchain.all_files],
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
    },
    toolchains = [
        "@rules_bison//bison:toolchain_type",
    ],
)
