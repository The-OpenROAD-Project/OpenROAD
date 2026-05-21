# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Tiny shim that exposes an orfs_pdk target's PdkInfo files (sources +
# Liberty set) as a normal file-providing target. Used by the
# integrated-synthesis smoke test to plumb the asap7 LEF / lib paths
# into a vanilla genrule, since @orfs//flow's BUILD does not export
# those source files publicly in the current archive.

load("@bazel-orfs//:openroad.bzl", "PdkInfo")

def _pdk_libs_impl(ctx):
    libs = ctx.attr.pdk[PdkInfo].libs
    return [DefaultInfo(files = libs)]

pdk_libs = rule(
    implementation = _pdk_libs_impl,
    attrs = {
        "pdk": attr.label(
            mandatory = True,
            providers = [PdkInfo],
        ),
    },
)

def _pdk_files_impl(ctx):
    info = ctx.attr.pdk[PdkInfo]
    return [DefaultInfo(files = depset(transitive = [info.files, info.libs]))]

pdk_files = rule(
    implementation = _pdk_files_impl,
    attrs = {
        "pdk": attr.label(
            mandatory = True,
            providers = [PdkInfo],
        ),
    },
)
