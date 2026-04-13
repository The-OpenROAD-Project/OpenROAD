# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026-2026, Precision Innovations Inc.

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "db",
    srcs = glob([
        "*.cpp",
        "*.h",
        "*.hpp",
        "*.inc",
    ]),
    features = [
        "-use_header_modules",
    ],
    deps = [
        "//src/odb:headers",
        # db source files include odb/lefout.h to implement writeLef()
        # overloads that only call inline methods. Depending on the
        # headers-only sub-target avoids a link cycle with :lefout.
        "//src/odb/src/lefout:lefout_headers",
        "//src/odb/src/zutil",
        "//src/utl",
        "@abseil-cpp//absl/synchronization",
        "@boost.container",
        "@boost.geometry",
        "@boost.integer",
        "@boost.polygon",
        "@boost.property_tree",
        "@boost.regex",
        "@spdlog",
    ],
)
