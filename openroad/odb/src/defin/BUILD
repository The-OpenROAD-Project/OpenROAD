# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026-2026, Precision Innovations Inc.

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//test:regression.bzl", "message_srcs")

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
    hdrs = ["//openroad/odb:include/odb/defin.h"],
    includes = [
        "../../include",
    ],
    deps = [
        "//openroad/odb/src/db",
        "//openroad/odb/src/def",
        "//openroad/odb/src/def:defzlib",
        "//openroad/utl",
        "@abseil-cpp//absl/base:core_headers",
        "@abseil-cpp//absl/synchronization",
        "@boost.algorithm",
    ],
)

message_srcs(
    visibility = ["//openroad/odb:__pkg__"],
)
