# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026-2026, Precision Innovations Inc.

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

# Header-only split so //src/odb/src/db can include odb/lefout.h (it
# implements lefout::writeLef() overloads that call inline methods on
# the lefout writer) without forming a link cycle: :db depends on
# :lefout_headers, while :lefout (the implementation) depends on :db.
cc_library(
    name = "lefout_headers",
    hdrs = ["//src/odb:include/odb/lefout.h"],
    features = [
        "-use_header_modules",
    ],
    deps = [
        "//src/odb:headers",
        "//src/utl",
        "@boost.polygon",
    ],
)

cc_library(
    name = "lefout",
    srcs = glob(["*.cpp"]),
    # Republish lefout.h so ordinary consumers that depend on :lefout see
    # the header (layering_check does not forward hdrs through deps). The
    # header also appears in :lefout_headers for :db's compile-only need.
    hdrs = ["//src/odb:include/odb/lefout.h"],
    features = [
        "-use_header_modules",
    ],
    deps = [
        ":lefout_headers",
        "//src/odb:headers",
        "//src/odb/src/db",
        "//src/utl",
        "@boost.polygon",
        "@spdlog",
    ],
)
