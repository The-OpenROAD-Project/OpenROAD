"""
This module provides a rule to promote a binary to a cfg=exec
to enable its use in sh_test(), this is to avoid building
OpenROAD twice, once with -c opt cfg=exec and once with with
-c opt target
"""

def _toolify(ctx):
    # Declare the output script
    output_script = ctx.actions.declare_file(ctx.attr.bin.label.name + ".sh")

    # Write the script content
    ctx.actions.write(
        output = output_script,
        content = """#!/bin/bash
set -e
exec "../{path}" "$@"
""".format(path = ctx.executable.bin.short_path),
        is_executable = True,
    )

    return DefaultInfo(
        executable = output_script,
        runfiles = ctx.runfiles(
            [],
            transitive_files = depset(
                [ctx.executable.bin],
                transitive = [
                    ctx.attr.bin.default_runfiles.files,
                    ctx.attr.bin.default_runfiles.symlinks,
                ],
            ),
        ),
    )

toolify = rule(
    implementation = _toolify,
    attrs = {
        "bin": attr.label(
            executable = True,
            cfg = "exec",
            allow_files = True,
        ),
    },
    executable = True,
)
