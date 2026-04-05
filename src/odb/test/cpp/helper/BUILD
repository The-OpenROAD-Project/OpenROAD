# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    features = ["layering_check"],
)

cc_library(
    name = "helper",
    testonly = 1,
    srcs = [
        "helper.cpp",
    ],
    hdrs = [
        "helper.h",
    ],
    includes = ["."],  # not needed after no cmake
    visibility = ["//visibility:public"],
    deps = [
        "//src/odb",
        "//src/tst",
        "//src/utl",
    ],
)
