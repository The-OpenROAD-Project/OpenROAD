# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

# Bazel-built Kokkos and KokkosFFT for the opt-in GPU build. This package
# holds the BUILD overlays for the pinned source archives (BUILD.kokkos.bazel,
# BUILD.kokkos_fft.bazel, referenced from MODULE.bazel) and configure.bzl,
# which the overlays load to write the configuration headers their CMake
# builds would have generated. It lives under bazel/gpu because Kokkos
# exists in this repo only as part of --config=gpu; the overlay targets are
# reached solely through @kokkos / @kokkos_fft from GPU-only targets.

exports_files(
    [
        "BUILD.kokkos.bazel",
        "BUILD.kokkos_fft.bazel",
    ],
    visibility = ["//visibility:public"],
)
