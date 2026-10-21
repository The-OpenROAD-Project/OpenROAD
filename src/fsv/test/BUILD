# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

package(features = ["layering_check"])

cc_test(
    name = "TestCubeGreens",
    srcs = ["cpp/TestCubeGreens.cpp"],
    deps = [
        "//src/fsv:core",
        "//src/fsv:geometry",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestSpatialIndex",
    srcs = ["cpp/TestSpatialIndex.cpp"],
    deps = [
        "//src/fsv:core",
        "//src/fsv:geometry",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestItf",
    srcs = ["cpp/TestItf.cpp"],
    data = ["sky130.itf"],
    deps = [
        "//src/fsv:stack",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestSolver",
    size = "medium",
    srcs = ["cpp/TestSolver.cpp"],
    deps = [
        "//src/fsv",
        "//src/fsv:geometry",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_binary(
    name = "BenchCube",
    srcs = ["cpp/BenchCube.cpp"],
    deps = [
        "//src/fsv",
        "//src/fsv:geometry",
    ],
)

cc_binary(
    name = "BenchGrid",
    srcs = ["cpp/BenchGrid.cpp"],
    deps = [
        "//src/fsv",
        "//src/fsv:geometry",
    ],
)
