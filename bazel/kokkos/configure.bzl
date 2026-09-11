# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Configuration headers for the Bazel-built Kokkos dependency."""

load("@bazel_skylib//rules:common_settings.bzl", "BuildSettingInfo")

_ARCH_DEFINES = {
    "sm_100": ["KOKKOS_ARCH_BLACKWELL", "KOKKOS_ARCH_BLACKWELL100"],
    "sm_103": ["KOKKOS_ARCH_BLACKWELL", "KOKKOS_ARCH_BLACKWELL103"],
    "sm_120": ["KOKKOS_ARCH_BLACKWELL", "KOKKOS_ARCH_BLACKWELL120"],
    "sm_121": ["KOKKOS_ARCH_BLACKWELL", "KOKKOS_ARCH_BLACKWELL121"],
    "sm_60": ["KOKKOS_ARCH_PASCAL", "KOKKOS_ARCH_PASCAL60"],
    "sm_61": ["KOKKOS_ARCH_PASCAL", "KOKKOS_ARCH_PASCAL61"],
    "sm_70": ["KOKKOS_ARCH_VOLTA", "KOKKOS_ARCH_VOLTA70"],
    "sm_72": ["KOKKOS_ARCH_VOLTA", "KOKKOS_ARCH_VOLTA72"],
    "sm_75": ["KOKKOS_ARCH_TURING75"],
    "sm_80": ["KOKKOS_ARCH_AMPERE", "KOKKOS_ARCH_AMPERE80"],
    "sm_86": ["KOKKOS_ARCH_AMPERE", "KOKKOS_ARCH_AMPERE86"],
    "sm_87": ["KOKKOS_ARCH_AMPERE", "KOKKOS_ARCH_AMPERE87"],
    "sm_89": ["KOKKOS_ARCH_ADA89"],
    "sm_90": ["KOKKOS_ARCH_HOPPER", "KOKKOS_ARCH_HOPPER90"],
}

def _kokkos_config_header_impl(ctx):
    arch = ctx.attr.cuda_arch[BuildSettingInfo].value
    defines = _ARCH_DEFINES.get(arch)
    if defines == None:
        fail(
            "--config=gpu requires an explicit CUDA architecture; use " +
            "--config=gpu-sm120, --config=gpu-sm121, or pass " +
            "--//:cuda_arch=sm_<compute capability>",
        )

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
