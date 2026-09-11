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

CUDA_TOOLKIT_COPTS = [
    # Overrides the global "-xc++" from .bazelrc: rule copts come after
    # --cxxopt on the command line and the last -x before the input wins.
    "-xcuda",
    # Execroot-relative repo path, so the toolkit headers reach the
    # compiler as declared inputs (an absolute system path would trip
    # Bazel's undeclared-inclusion check).
    "--cuda-path=" + CUDA_PATH,
    # CUDA 13 moved the libcu++ headers that Kokkos includes (cuda/std/*)
    # from include/ down into include/cccl/. Harmless on 12.x.
    "-isystem",
    CUDA_PATH + "/include/cccl",
    "-Wno-unknown-cuda-version",
    # The hermetic LLVM toolchain uses libc++; CUDA's host header requires
    # this opt-out on x86_64.
    "-D_ALLOW_UNSUPPORTED_LIBCPP",
]

CUDA_ARCH_COPTS = select(
    {
        Label("//:cuda_arch_" + arch): ["--cuda-gpu-arch=" + arch]
        for arch in [
            "sm_60",
            "sm_61",
            "sm_70",
            "sm_72",
            "sm_75",
            "sm_80",
            "sm_86",
            "sm_87",
            "sm_89",
            "sm_90",
            "sm_100",
            "sm_103",
            "sm_120",
            "sm_121",
        ]
    },
    no_match_error = (
        "--config=gpu requires an explicit CUDA architecture; use " +
        "--config=gpu-sm120, --config=gpu-sm121, or pass " +
        "--//:cuda_arch=sm_<compute capability>"
    ),
)

CUDA_COPTS = CUDA_TOOLKIT_COPTS + CUDA_ARCH_COPTS + [
    # Device-side FMA off for bit-stable results — the CMake --fmad=false
    # equivalent. The global .bazelrc -ffp-contract=off also reaches these
    # TUs; this restates it at rule level so device code stays FMA-free
    # even if the global cxxopt is ever dropped (clang's CUDA default is
    # fp-contract=fast).
    "-ffp-contract=off",
    # libc++'s placement new/delete are __host__ only, which breaks the
    # Kokkos kernels that placement-new in __host__ __device__ code. The
    # shim replaces them with host+device definitions; see the header.
    # Resolved through $(location) so the path is right in any execroot,
    # including a downstream module's where this repo lives under
    # external/; every user of CUDA_COPTS must list the header in
    # additional_compiler_inputs, which is what makes the expansion legal.
    "-include",
    "$(location //bazel/gpu:cuda_placement_new.h)",
    # gpl's sources use OpenMP pragmas on the host side; same flag (and
    # @openmp dependency) as the CPU //src/gpl target.
    "-fopenmp",
]
