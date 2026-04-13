# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026-2026, Precision Innovations Inc.

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "zutil",
    srcs = glob(["*.cpp"]),
    features = [
        "-use_header_modules",
    ],
    deps = [
        "//src/odb:headers",
        "//src/utl",
        "@boost.polygon",
    ],
)
