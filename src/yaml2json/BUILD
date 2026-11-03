# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

package(features = ["layering_check"])

cc_library(
    name = "lib",
    srcs = ["yaml2json.cc"],
    hdrs = ["yaml2json.h"],
    deps = ["@yaml-cpp"],
)

# Converts YAML to JSON for scripts that should not need a YAML library on
# the host, such as ORFS's readers of flow/scripts/variables.yaml. Installed
# next to openroad by //:install.
cc_binary(
    name = "yaml2json",
    srcs = ["main.cc"],
    visibility = ["//visibility:public"],
    deps = [":lib"],
)

cc_test(
    name = "yaml2json_test",
    srcs = ["yaml2json_test.cc"],
    deps = [
        ":lib",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
        "@yaml-cpp",
    ],
)
