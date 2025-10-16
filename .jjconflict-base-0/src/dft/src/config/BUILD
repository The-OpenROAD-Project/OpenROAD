# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "config",
    srcs = [
        "DftConfig.cpp",
        "ScanArchitectConfig.cpp",
        "ScanStitchConfig.cpp",
    ],
    hdrs = [
        "DftConfig.hh",
        "ScanArchitectConfig.hh",
        "ScanStitchConfig.hh",
    ],
    includes = [
        ".",
    ],
    deps = [
        "//src/dft/src/utils",
        "//src/utl",
    ],
)
