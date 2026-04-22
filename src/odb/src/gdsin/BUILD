# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026-2026, Precision Innovations Inc.

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "gdsin",
    srcs = glob(["*.cpp"]),
    hdrs = [
        "//src/odb:include/odb/gdsUtil.h",
        "//src/odb:include/odb/gdsin.h",
    ],
    deps = [
        "//src/odb/src/db",
        "//src/utl",
        "@boost.property_tree",
    ],
)

filegroup(
    name = "message_srcs",
    srcs = glob(["*.cpp"]),
    visibility = ["//src/odb:__pkg__"],
)
