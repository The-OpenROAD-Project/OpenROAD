# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("@rules_python//python:defs.bzl", "py_test")
load("//test:regression.bzl", "doc_check_test", "regression_test")

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
    name = "TestDielectric",
    srcs = ["cpp/TestDielectric.cpp"],
    deps = [
        "//src/fsv:core",
        "//src/fsv:geometry",
        "//src/fsv:stack",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestSquareGreens",
    srcs = ["cpp/TestSquareGreens.cpp"],
    deps = [
        "//src/fsv:core",
        "//src/fsv:geometry",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestLayeredCubeGreens",
    srcs = ["cpp/TestLayeredCubeGreens.cpp"],
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
    name = "TestGolden",
    size = "large",
    srcs = ["cpp/TestGolden.cpp"],
    data = glob(["golden/*.txt"]),
    shard_count = 4,
    deps = [
        "//src/fsv",
        "//src/fsv:geometry",
        "//src/fsv:stack",
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
        "//src/fsv:core",
        "//src/fsv:geometry",
        "//src/fsv:stack",
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

TESTS = [
    "field_solver_cross_section",
    "field_solver_errors",
    "field_solver_itf",
    "field_solver_uniform",
]

filegroup(
    name = "test_resources",
    srcs = glob(
        ["**/*"],
        exclude = [
            test + "." + ext
            for test in TESTS
            for ext in [
                "tcl",
                "py",
            ]
        ],
    ),
)

[regression_test(
    name = test_name,
    data = [":test_resources"],
) for test_name in TESTS]

py_test(
    name = "fsv_man_tcl_check",
    srcs = ["fsv_man_tcl_check.py"],
    data = ["//src/fsv:doc_files"],
    tags = [
        "doc_check",
        "py",
    ],
    deps = ["//docs/src/scripts:extract_utils"],
)

doc_check_test(
    name = "fsv_readme_msgs_check",
    data = [
        "//src/fsv:doc_files",
        "//src/fsv:messages_txt",
    ],
)
