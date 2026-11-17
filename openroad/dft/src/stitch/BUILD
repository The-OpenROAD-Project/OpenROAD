# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//test:regression.bzl", "message_srcs")

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
        "//openroad/dft/src/architect",
        "//openroad/dft/src/cells",
        "//openroad/dft/src/config",
        "//openroad/dft/src/utils",
        "//openroad/odb/src/db",
        "//openroad/utl",
        "@boost.algorithm",
    ],
)

message_srcs(
    visibility = ["//openroad/dft:__pkg__"],
)
