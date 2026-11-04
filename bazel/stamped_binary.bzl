# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Writes the release version into a binary after it is linked.

The version changes with every commit. Compiled in, it would change the link
on every commit, so the link could never come from the remote cache, and a
local link has to download every object file first. Instead the binary links
against a placeholder version slot (bazel/version/version.cc), and under
--stamp one cheap action writes the version into a copy of the linked binary.
Without --stamp the output is a symlink to the linked binary.
"""

def _stamped_binary_impl(ctx):
    binary = ctx.executable.binary
    out = ctx.actions.declare_file(ctx.label.name)

    if ctx.attr.stamp:
        args = ctx.actions.args()
        args.add(binary)
        args.add(ctx.info_file)
        args.add(out)
        if ctx.attr.codesign:
            args.add("--codesign")
        ctx.actions.run(
            executable = ctx.attr._stamp_version[DefaultInfo].files_to_run,
            arguments = [args],
            inputs = [binary, ctx.info_file],
            outputs = [out],
            mnemonic = "StampVersion",
            progress_message = "Stamping the version into %{output}",
        )
    else:
        ctx.actions.symlink(
            output = out,
            target_file = binary,
            is_executable = True,
        )

    # Keep the binary's data files (e.g. the ICU data of the GUI build) but
    # not the binary itself, which would otherwise ship next to `out`.
    data = [
        f
        for f in ctx.attr.binary[DefaultInfo].default_runfiles.files.to_list()
        if f != binary
    ]
    providers = [
        DefaultInfo(
            executable = out,
            files = depset([out]),
            runfiles = ctx.runfiles(files = [out] + data),
        ),
        coverage_common.instrumented_files_info(
            ctx,
            dependency_attributes = ["binary"],
        ),
    ]
    if RunEnvironmentInfo in ctx.attr.binary:
        providers.append(ctx.attr.binary[RunEnvironmentInfo])
    return providers

_stamped_binary = rule(
    implementation = _stamped_binary_impl,
    executable = True,
    attrs = {
        "binary": attr.label(
            doc = "The linked binary, carrying the placeholder version slot.",
            executable = True,
            cfg = "target",
            mandatory = True,
        ),
        "codesign": attr.bool(
            doc = "Re-sign ad hoc after stamping, as macOS requires.",
            default = False,
        ),
        "stamp": attr.bool(
            doc = "Write the version. Set it from //bazel:stamp so it " +
                  "follows --stamp.",
            default = False,
        ),
        "_stamp_version": attr.label(
            default = "//bazel:stamp_version",
            executable = True,
            cfg = "exec",
        ),
    },
)

def stamped_binary(name, binary, **kwargs):
    """Declares `name`: `binary` with the version written in under --stamp.

    Args:
      name: Target name, and the output file name.
      binary: The cc_binary to stamp. It must depend on
        //bazel:openroad_version.
      **kwargs: Common rule attributes such as visibility.
    """
    _stamped_binary(
        name = name,
        binary = binary,
        stamp = select({
            "//bazel:stamp": True,
            "//conditions:default": False,
        }),
        codesign = select({
            "@platforms//os:macos": True,
            "//conditions:default": False,
        }),
        **kwargs
    )
