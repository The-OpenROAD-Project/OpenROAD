# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026-2026, Precision Innovations Inc.

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//test:regression.bzl", "message_srcs")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "gdsout",
    srcs = glob(["*.cpp"]),
    hdrs = ["//src/odb:include/odb/gdsout.h"],
    includes = [
        "../../include",
    ],
    deps = [
        "//src/odb/src/db",
        "//src/odb/src/gdsin",  # for gdsUtil.h
        "//src/utl",
    ],
)

message_srcs(
    visibility = ["//src/odb:__pkg__"],
)
