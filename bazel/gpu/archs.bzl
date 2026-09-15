# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""CUDA architectures supported by the Bazel GPU build.

One dict maps each --//:cuda_arch value to the Kokkos_ARCH_* macros Kokkos's
CMake would define for it. Keeping the sm_* list and the Kokkos define map
together guarantees that the flag's accepted values (BUILD.bazel), the
--cuda-gpu-arch select (copts.bzl) and the generated KokkosCore_config.h
(bazel/kokkos/configure.bzl) always cover the same set.
"""

CUDA_ARCH_DEFINES = {
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

CUDA_ARCHS = sorted(CUDA_ARCH_DEFINES.keys())

CUDA_ARCH_FLAG_ERROR = (
    "--config=gpu requires an explicit CUDA architecture: pass " +
    "--@openroad//:cuda_arch=sm_<compute capability> (--//:cuda_arch=... " +
    "inside the OpenROAD checkout). See docs/user/Bazel.md, section " +
    "\"GPU build\"."
)
