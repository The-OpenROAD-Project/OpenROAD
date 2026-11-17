# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//test:regression.bzl", "message_srcs")

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
        "//openroad/dbSta",
        "//openroad/dbSta:dbNetwork",
        "//openroad/odb/src/db",
        "//openroad/sta:opensta_lib",
        "//openroad/utl",
        "@spdlog",
    ],
)

message_srcs(
    visibility = ["//openroad/dft:__pkg__"],
)
