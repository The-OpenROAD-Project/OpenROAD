# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "stitch",
    srcs = [
        "ScanStitch.cpp",
    ],
    hdrs = [
        "ScanStitch.hh",
    ],
    includes = [
        ".",
    ],
    deps = [
        "//src/dft/src/architect",
        "//src/dft/src/cells",
        "//src/dft/src/config",
        "//src/dft/src/utils",
        "//src/odb",
        "//src/utl",
        "@boost.algorithm",
    ],
)
