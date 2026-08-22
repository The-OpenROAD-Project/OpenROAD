# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "optimizer",
    srcs = [
        "OdbScanCellAdapter.cpp",
        "Opt.cpp",
    ],
    hdrs = [
        "KMeans.hh",
        "OdbScanCellAdapter.hh",
        "Opt.hh",
    ],
    includes = [
        ".",
    ],
    deps = [
        "//src/dft/src/cells",
        "//src/dft/src/clock_domain",
        "//src/odb",
        "//src/utl",
        "@boost.geometry",
    ],
)
