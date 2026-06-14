# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026-2026, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "mockturtle",
    hdrs = glob(["mockturtle/include/mockturtle/**/*.hpp"]) + glob([
        "mockturtle/lib/kitty/**/*.hpp",
    ]) + glob([
        "mockturtle/lib/lorina/**/*.hpp",
    ]) + glob([
        "mockturtle/lib/lorina/**/*.inc",
    ]),
    includes = [
        "mockturtle/include/",
        "mockturtle/lib/kitty/",
        "mockturtle/lib/lorina/",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@abseil-cpp//absl/base:core_headers",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/random:random",
        "@spdlog",
    ],
)
