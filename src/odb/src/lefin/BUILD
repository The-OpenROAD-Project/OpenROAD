# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026-2026, Precision Innovations Inc.

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "lefin",
    srcs = glob([
        "*.cpp",
        "*.h",
    ]),
    hdrs = ["//src/odb:include/odb/lefin.h"],
    deps = [
        "//src/odb/src/db",
        "//src/odb/src/lef",
        "//src/odb/src/lef:lefzlib",
        "//src/utl",
        "@abseil-cpp//absl/base:core_headers",
        "@abseil-cpp//absl/synchronization",
        "@boost.algorithm",
        "@boost.bind",
        "@boost.config",
        "@boost.fusion",
        "@boost.lambda",
        "@boost.optional",
        "@boost.phoenix",
        "@boost.spirit",
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
