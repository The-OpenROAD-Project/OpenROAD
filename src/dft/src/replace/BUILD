# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "replace",
    srcs = [
        "ScanReplace.cpp",
    ],
    hdrs = [
        "ScanReplace.hh",
    ],
    includes = [
        ".",
    ],
    deps = [
        "//src/dbSta",
        "//src/dbSta:dbNetwork",
        "//src/dft/src/utils",
        "//src/odb",
        "//src/sta:opensta_lib",
        "//src/utl",
    ],
)
