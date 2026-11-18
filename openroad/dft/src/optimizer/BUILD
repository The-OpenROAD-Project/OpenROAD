# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//test:regression.bzl", "message_srcs")

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
        "//openroad/dft/src/cells",
        "//openroad/dft/src/clock_domain",
        "//openroad/dft/src/utils",
        "//openroad/odb/src/db",
        "//openroad/utl",
        "@boost.geometry",
    ],
)

message_srcs(
    visibility = ["//openroad/dft:__pkg__"],
)
