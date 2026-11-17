# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026-2026, Precision Innovations Inc.

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//test:regression.bzl", "message_srcs")

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
    hdrs = ["//openroad/odb:include/odb/lefin.h"],
    includes = [
        "../../include",
    ],
    deps = [
        "//openroad/odb/src/db",
        "//openroad/odb/src/lef",
        "//openroad/odb/src/lef:lefzlib",
        "//openroad/utl",
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

message_srcs(
    visibility = ["//openroad/odb:__pkg__"],
)
