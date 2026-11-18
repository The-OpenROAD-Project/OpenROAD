# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026-2026, Precision Innovations Inc.

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//test:regression.bzl", "message_srcs")

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
    hdrs = ["//openroad/odb:include/odb/3dblox.h"],
    includes = [
        "../../include",
    ],
    deps = [
        "//openroad/odb/src/db",
        "//openroad/odb/src/defin",
        "//openroad/odb/src/defout",
        "//openroad/odb/src/lefin",
        "//openroad/odb/src/lefout",
        "//openroad/sta:opensta_lib",
        "//openroad/utl",
        "@spdlog",
        "@yaml-cpp",
    ],
)

message_srcs(
    visibility = ["//openroad/odb:__pkg__"],
)
