# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2020-2026, The OpenROAD Authors

"""Build rule for generating C or C++ sources with Flex."""

def _correct_flex_env_for_action(env, flex):
    """Modify the flex environment variables to work in an action that doesn't a have built flex runfiles directory.

    The `flex_toolchain.flex_env` parameter assumes that flex will provided via an executable attribute
    and thus have built runfiles available to it. This is not the case for this action and any other actions
    trying to use flex as a tool via the toolchain. This function transforms existing environment variables
    to support running Flex as desired.

    Args:
        env (dict): The existing Flex environment variables
        flex (File): The Flex executable

    Returns:
        Dict: Environment variables required for running Flex.
    """
    flex_env = dict(env)

    # Convert the environment variables to non-runfiles forms
    flex_runfiles_dir = "{}.runfiles/{}".format(
        flex.path,
        flex.owner.workspace_name,
    )

    actual = "{}/external/{}".format(flex.root.path, flex.owner.workspace_name)

    for key, value in flex_env.items():
        flex_env[key] = value.replace(flex_runfiles_dir, actual)

    return flex_env

def _genlex_impl(ctx):
    """Implementation for genlex rule."""

    flex_toolchain = ctx.toolchains["@rules_flex//flex:toolchain_type"].flex_toolchain

    # Compute the prefix, if not specified.
    if ctx.attr.prefix:
        prefix = ctx.attr.prefix
    else:
        prefix = ctx.file.src.basename.partition(".")[0]

    # Construct the arguments.
    args = ctx.actions.args()
    args.add("-o", ctx.outputs.out)
    outputs = [ctx.outputs.out]
    if ctx.outputs.header_out:
        args.add(ctx.outputs.header_out.path, format = "--header-file=%s")
        outputs.append(ctx.outputs.header_out)
    args.add("-P", prefix)
    args.add_all(ctx.attr.lexopts)
    args.add(ctx.file.src)

    flex_env = _correct_flex_env_for_action(
        env = flex_toolchain.flex_env,
        flex = flex_toolchain.flex_tool.executable,
    )

    ctx.actions.run(
        executable = flex_toolchain.flex_tool.executable,
        env = flex_env,
        arguments = [args],
        inputs = ctx.files.src + ctx.files.includes,
        tools = [flex_toolchain.all_files],
        outputs = outputs,
        mnemonic = "Flex",
        progress_message = "Generating %s from %s" % (
            ctx.outputs.out.short_path,
            ctx.file.src.short_path,
        ),
    )

genlex = rule(
    implementation = _genlex_impl,
    doc = """\
Generate C/C++-language sources from a lex file using Flex.

IMPORTANT: we _strongly recommend_ that you include a unique and project-
specific `%option prefix="myproject"` directive in your scanner spec to avoid
very hard-to-debug symbol name conflict problems if two scanners are linked
into the same dynamically-linked executable.  Consider using ANTLR for new
projects.
By default, flex includes the definition of a static function `yyunput` in its
output. If you never use the lex `unput` function in your lex rules, however,
`yyunput` will never be called. This causes problems building the output file,
as llvm issues warnings about static functions that are never called. To avoid
this problem, use `%option nounput` in the declarations section if your lex
rules never use `unput`.
Note that if you use the c++ mode of flex, you will need to include the
boilerplate header `FlexLexer.h` file in any `cc_library` which includes the
generated flex scanner directly.  This is typically done by
`#include <FlexLexer.h>` with a declared BUILD dependency on
`@com_github_westes_flex//:FlexLexer`.
Flex invokes m4 behind the scenes to generate the output scanner.  As such,
all genlex rules have an implicit dependency on `@org_gnu_m4//:m4`.  Note
also that certain M4 control sequences (notably exactly the strings `"[["` and
`"]]"`) are not correctly handled by flex as a result.

Examples
--------
This is a simple example.
```python
genlex(
    name = "html_lex_lex",
    src = "html.lex",
    out = "html_lexer.c",
)
```

This example uses a `.tab.hh` file.
```python
genlex(
    name = "rules_l",
    src = "rules.lex",
    includes = [
        "rules.tab.hh",
    ],
    out = "rules.yy.cc",
)
```
""",
    attrs = {
        "header_out": attr.output(
            mandatory = False,
            doc = "The generated header file",
        ),
        "includes": attr.label_list(
            allow_files = True,
            doc = "A list of headers that are included by the .lex file",
        ),
        "lexopts": attr.string_list(
            doc = "A list of options to be added to the flex command line.",
        ),
        "out": attr.output(
            doc = "The generated source file",
            mandatory = True,
        ),
        "prefix": attr.string(
            doc = "External symbol prefix for Flex. This string is " +
                  "passed to flex as the -P option, causing the resulting C " +
                  "file to define external functions named 'prefix'text, " +
                  "'prefix'in, etc.  The default is the basename of the source" +
                  "file without the .lex extension.",
            default = "yy",
        ),
        "src": attr.label(
            mandatory = True,
            allow_single_file = [".l", ".ll", ".lex", ".lpp"],
            doc = "The .lex source file for this rule",
        ),
    },
    toolchains = [
        "@rules_flex//flex:toolchain_type",
    ],
)
