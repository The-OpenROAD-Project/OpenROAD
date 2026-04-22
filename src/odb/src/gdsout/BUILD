# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026-2026, Precision Innovations Inc.

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "gdsout",
    srcs = glob(["*.cpp"]),
    hdrs = ["//src/odb:include/odb/gdsout.h"],
    deps = [
        "//src/odb/src/db",
        "//src/odb/src/gdsin",  # for gdsUtil.h
        "//src/utl",
    ],
)

filegroup(
    name = "message_srcs",
    srcs = glob(["*.cpp"]),
    visibility = ["//src/odb:__pkg__"],
)
