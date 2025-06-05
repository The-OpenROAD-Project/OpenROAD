# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("//bazel:tcl_encode_or.bzl", "tcl_encode")
load("//bazel:tcl_wrap_cc.bzl", "tcl_wrap_cc")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "cells",
    srcs = [
        "OneBitScanCell.cpp",
        "ScanCell.cpp",
        "ScanCellFactory.cpp",
    ],
    hdrs = [
        "OneBitScanCell.hh",
        "ScanCell.hh",
        "ScanCellFactory.hh",
    ],
    includes = [
        ".",
    ],
    deps = [
        "//:opensta_lib",
        "//src/dbSta",
        "//src/dft/src/clock_domain",
        "//src/dft/src/config",
        "//src/dft/src/utils",
        "//src/odb",
        "//src/utl",
    ],
)
