# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

"""Compile flags for CUDA translation units in the opt-in GPU build.

The hermetic clang compiles these TUs in its native CUDA mode against the
system toolkit wrapped by @cuda_local; no separate device compiler or
ruleset is involved. See bazel/gpu/system_gpu.bzl.
"""

load("@cuda_local//:defs.bzl", "CUDA_ARCH", "CUDA_PATH")

CUDA_COPTS = [
    # Overrides the global "-xc++" from .bazelrc: rule copts come after
    # --cxxopt on the command line and the last -x before the input wins.
    "-xcuda",
    # Execroot-relative repo path, so the toolkit headers reach the
    # compiler as declared inputs (an absolute system path would trip
    # Bazel's undeclared-inclusion check).
    "--cuda-path=" + CUDA_PATH,
    "--cuda-gpu-arch=" + CUDA_ARCH,
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
    "-fopenmp",
]
