# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026-2026, Precision Innovations Inc.

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "3dblox",
    srcs = glob([
        "*.cpp",
        "*.h",
    ]),
    hdrs = ["//src/odb:include/odb/3dblox.h"],
    deps = [
        "//src/odb/src/db",
        "//src/odb/src/defin",
        "//src/odb/src/defout",
        "//src/odb/src/lefin",
        "//src/odb/src/lefout",
        "//src/sta:opensta_lib",
        "//src/utl",
        "@yaml-cpp",
    ],
)

filegroup(
    name = "message_srcs",
    srcs = glob([
        "*.cpp",
        "*.h",
    ]),
    visibility = ["//src/odb:__pkg__"],
)
