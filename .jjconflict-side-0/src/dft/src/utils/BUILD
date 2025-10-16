# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "utils",
    srcs = [
        "ScanPin.cpp",
        "Utils.cpp",
    ],
    hdrs = [
        "Formatting.hh",
        "ScanPin.hh",
        "Utils.hh",
    ],
    includes = [
        ".",
    ],
    deps = [
        "//src/dbSta",
        "//src/dbSta:dbNetwork",
        "//src/odb",
        "//src/sta:opensta_lib",
        "//src/utl",
        "@spdlog",
    ],
)
