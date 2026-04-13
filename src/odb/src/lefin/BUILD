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
    features = [
        "-use_header_modules",
    ],
    deps = [
        "//src/odb:headers",
        "//src/odb/src/db",
        "//src/odb/src/lef",
        "//src/odb/src/lef:lefzlib",
        "//src/odb/src/zutil",
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
