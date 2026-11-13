# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

# Bazel-built Kokkos and KokkosFFT for the opt-in GPU build.
#
# Kokkos and KokkosFFT both come from the registry (bazel_dep kokkos,
# kokkos_fft). Under --config=gpu, .bazelrc configures them from this
# package:
#   --@kokkos//:cuda_toolkit=:kokkos_cuda_toolkit  (the system CUDA toolkit)
#   --@kokkos_fft//:config=:kokkos_fft_config      (KokkosFFT_config.hpp)
# together with --@kokkos//:device=cuda and --@kokkos//:cuda_arch.
#
# configure.bzl writes KokkosFFT_config.hpp the way KokkosFFT's CMake build
# would have generated it.

load("@cuda_local//:defs.bzl", "CUDA_PATH")
load("@kokkos//:cuda_toolkit.bzl", "kokkos_cuda_toolkit")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load(":configure.bzl", "kokkos_fft_config")

# KokkosFFT_config.hpp, written at build time from the backend list in
# configure.bzl. It matches a KokkosFFT CMake configure with
# KokkosFFT_ENABLE_FFTW=ON (gpl runs its FFTs on the host).
#
# @fftw does not build FFTW's OpenMP/threads library. A plan on
# Kokkos::DefaultHostExecutionSpace (Kokkos::OpenMP here) calls
# fftw_init_threads and would fail to link, so host FFTs must use
# Kokkos::Serial (see src/gpl/src/gpu/dct.cpp).
kokkos_fft_config(name = "kokkos_fft_config_hdr")

# Target for --@kokkos_fft//:config (set under --config=gpu in .bazelrc):
# the generated header plus cuFFT.
cc_library(
    name = "kokkos_fft_config",
    visibility = ["//visibility:public"],
    deps = [
        ":kokkos_fft_config_hdr",
        "@cuda_local//:cufft",
    ],
)

# KokkosFFT from the registry, configured through the flag above.
alias(
    name = "kokkos_fft",
    actual = "@kokkos_fft//:kokkos_fft",
    visibility = ["//visibility:public"],
)

# The system CUDA toolkit (@cuda_local), described for the registry Kokkos
# module's CUDA backend. Selected by --@kokkos//:cuda_toolkit in .bazelrc.
kokkos_cuda_toolkit(
    name = "kokkos_cuda_toolkit",
    compiler_inputs = ["@cuda_local//:compiler_inputs"],
    copts = [
        "--cuda-path=" + CUDA_PATH,
        # CUDA 13 moved the libcu++ headers (cuda/std/*) under include/cccl.
        # Harmless on 12.x.
        "-isystem",
        CUDA_PATH + "/include/cccl",
        "-Wno-unknown-cuda-version",
        # The hermetic LLVM toolchain uses libc++; CUDA's host header requires
        # this opt-out on x86_64.
        "-D_ALLOW_UNSUPPORTED_LIBCPP",
    ],
    runtime = "@cuda_local//:cudart",
    visibility = ["//visibility:public"],
)
