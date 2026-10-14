# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

package(default_visibility = ["//visibility:public"])

exports_files(["0001-add-bazel-build.patch"])

alias(
    name = "backend",
    actual = "@kokkos_host//:backend",
)

alias(
    name = "kokkos",
    actual = select({
        "//:gpu_cuda": "@kokkos",
        "//conditions:default": "@kokkos_host//:kokkos",
    }),
)
