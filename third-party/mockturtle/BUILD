# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026-2026, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

package(
    features = [
        "layering_check",
    ],
)

cc_library(
    name = "mockturtle",
    hdrs = glob(["include/mockturtle/**/*.hpp"]) + glob([
        "lib/kitty/**/*.hpp",
    ]) + glob([
        "lib/lorina/**/*.hpp",
    ]) + glob([
        "lib/lorina/**/*.inc",
    ]),
    includes = [
        "include/",
        "lib/kitty/",
        "lib/lorina/",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@abseil-cpp//absl/container:flat_hash_map",
        "@fmt",
    ],
)

cc_test(
    name = "mockturtle_emap_test",
    srcs = ["test/algorithms/emap.cpp"],
    visibility = ["//visibility:public"],
    deps = [
        ":mockturtle",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
