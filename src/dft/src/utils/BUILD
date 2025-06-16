# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")

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
        "//:opensta_lib",
        "//src/dbSta",
        "//src/odb",
        "//src/utl",
    ],
)
