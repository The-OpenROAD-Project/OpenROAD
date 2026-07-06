# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Compile flags for CUDA translation units in the opt-in GPU build.

The hermetic clang compiles these TUs in its native CUDA mode against the
system toolkit wrapped by @cuda_local; no separate device compiler or
ruleset is involved. See bazel/gpu/system_gpu.bzl.

cmake/KokkosBackend.cmake additionally defines FMT_USE_NONTYPE_TEMPLATE_ARGS=0
and, on aarch64, BOOST_UNORDERED_DISABLE_NEON for its CUDA TUs. Both work
around nvcc front-end limitations (fmt's consteval format strings, gcc's
arm_neon.h) that clang's CUDA mode does not have, so they are deliberately
absent here.
"""

load("@cuda_local//:defs.bzl", "CUDA_PATH")
load("@kokkos//:defs.bzl", "CUDA_ARCH")

CUDA_COPTS = [
    # Overrides the global "-xc++" from .bazelrc: rule copts come after
    # --cxxopt on the command line and the last -x before the input wins.
    "-xcuda",
    # Execroot-relative repo path, so the toolkit headers reach the
    # compiler as declared inputs (an absolute system path would trip
    # Bazel's undeclared-inclusion check).
    "--cuda-path=" + CUDA_PATH,
] + (
    # The compute capability the Kokkos install was built for (read from
    # its KokkosCore_config.h by system_gpu.bzl). Empty only when @kokkos
    # is a stub, in which case analysis fails on the stub target before
    # any CUDA TU is compiled.
    ["--cuda-gpu-arch=" + CUDA_ARCH] if CUDA_ARCH else []
) + [
    # CUDA 13 moved the libcu++ headers that Kokkos includes (cuda/std/*)
    # from include/ down into include/cccl/. Harmless on 12.x, where the
    # directory does not exist and clang drops the search path.
    "-isystem",
    CUDA_PATH + "/include/cccl",
    # clang only recognizes CUDA releases up to the one it was released
    # with; a newer toolkit (e.g. CUDA 13 with clang 22) works but would
    # otherwise warn once per TU.
    "-Wno-unknown-cuda-version",
    # CUDA's crt/host_defines.h rejects libc++ on x86_64 unless this
    # opt-out is set (the toolkit only "supports" libstdc++ there; the
    # guard does not exist on aarch64). The hermetic toolchain is
    # libc++-only, and clang's CUDA mode handles libc++ fine in practice.
    "-D_ALLOW_UNSUPPORTED_LIBCPP",
    # Device-side FMA off for bit-stable results — the CMake --fmad=false
    # equivalent. The global .bazelrc -ffp-contract=off also reaches these
    # TUs; this restates it at rule level so device code stays FMA-free
    # even if the global cxxopt is ever dropped (clang's CUDA default is
    # fp-contract=fast).
    "-ffp-contract=off",
    # libc++'s placement new/delete are __host__ only, which breaks the
    # Kokkos kernels that placement-new in __host__ __device__ code. The
    # shim replaces them with host+device definitions; see the header.
    "-include",
    "bazel/gpu/cuda_placement_new.h",
    # gpl's sources use OpenMP pragmas on the host side; same flag (and
    # @openmp dependency) as the CPU //src/gpl target.
    "-fopenmp",
]
