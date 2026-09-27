# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Configuration headers for the Bazel-built Kokkos dependency."""

load("@bazel_skylib//rules:common_settings.bzl", "BuildSettingInfo")
load("//bazel/gpu:archs.bzl", "CUDA_ARCH_DEFINES", "CUDA_ARCH_FLAG_ERROR")

def _kokkos_config_header_impl(ctx):
    arch = ctx.attr.cuda_arch[BuildSettingInfo].value
    defines = CUDA_ARCH_DEFINES.get(arch)
    if defines == None:
        fail(CUDA_ARCH_FLAG_ERROR)

    ctx.actions.expand_template(
        template = ctx.file.template,
        output = ctx.outputs.out,
        substitutions = {
            "@OPENROAD_KOKKOS_ARCH_DEFINES@": "\n".join(["#define " + define for define in defines]),
        },
    )
    return [DefaultInfo(files = depset([ctx.outputs.out]))]

kokkos_config_header = rule(
    implementation = _kokkos_config_header_impl,
    attrs = {
        "cuda_arch": attr.label(
            default = Label("//:cuda_arch"),
            providers = [BuildSettingInfo],
        ),
        "out": attr.output(mandatory = True),
        "template": attr.label(
            allow_single_file = True,
            default = "KokkosCore_config.h.tpl",
        ),
    },
)
