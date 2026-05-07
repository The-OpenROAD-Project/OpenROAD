# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026-2026, Precision Innovations Inc.

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "defin",
    srcs = glob([
        "*.cpp",
        "*.h",
    ]),
    hdrs = ["//src/odb:include/odb/defin.h"],
    deps = [
        "//src/odb/src/db",
        "//src/odb/src/def",
        "//src/odb/src/def:defzlib",
        "//src/utl",
        "@abseil-cpp//absl/base:core_headers",
        "@abseil-cpp//absl/synchronization",
        "@boost.algorithm",
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
