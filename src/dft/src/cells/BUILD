# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

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
        "//src/dbSta",
        "//src/dbSta:dbNetwork",
        "//src/dft/src/clock_domain",
        "//src/dft/src/config",
        "//src/dft/src/utils",
        "//src/odb",
        "//src/sta:opensta_lib",
        "//src/utl",
    ],
)
