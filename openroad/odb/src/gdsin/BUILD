# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026-2026, Precision Innovations Inc.

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//test:regression.bzl", "message_srcs")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "gdsin",
    srcs = glob(["*.cpp"]),
    hdrs = [
        "//openroad/odb:include/odb/gdsUtil.h",
        "//openroad/odb:include/odb/gdsin.h",
    ],
    includes = [
        "../../include",
    ],
    deps = [
        "//openroad/odb/src/db",
        "//openroad/utl",
        "@boost.property_tree",
    ],
)

message_srcs(
    visibility = ["//openroad/odb:__pkg__"],
)
