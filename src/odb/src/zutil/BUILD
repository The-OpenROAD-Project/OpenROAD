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
    hdrs = [
        "//src/odb:include/odb/poly_decomp.h",
        "//src/odb:include/odb/util.h",
    ],
    features = [
        "-use_header_modules",
    ],
    deps = [
        "//src/odb:headers",
        "//src/utl",
        "@boost.polygon",
    ],
)
