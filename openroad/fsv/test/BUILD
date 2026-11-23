# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("//bazel:man_tcl_checker.bzl", "man_tcl_check_test")
load("//test:regression.bzl", "doc_check_test", "regression_test")

package(features = ["layering_check"])

cc_test(
    name = "TestCubeGreens",
    srcs = ["cpp/TestCubeGreens.cpp"],
    deps = [
        "//openroad/fsv:core",
        "//openroad/fsv:geometry",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestDielectric",
    srcs = ["cpp/TestDielectric.cpp"],
    deps = [
        "//openroad/fsv:core",
        "//openroad/fsv:geometry",
        "//openroad/fsv:stack",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestSquareGreens",
    srcs = ["cpp/TestSquareGreens.cpp"],
    deps = [
        "//openroad/fsv:core",
        "//openroad/fsv:geometry",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestLayeredCubeGreens",
    srcs = ["cpp/TestLayeredCubeGreens.cpp"],
    deps = [
        "//openroad/fsv:core",
        "//openroad/fsv:geometry",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestGaussianSurface",
    srcs = ["cpp/TestGaussianSurface.cpp"],
    deps = [
        "//openroad/fsv:core",
        "//openroad/fsv:geometry",
        "//openroad/fsv:stack",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestSpatialIndex",
    srcs = ["cpp/TestSpatialIndex.cpp"],
    deps = [
        "//openroad/fsv:core",
        "//openroad/fsv:geometry",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

# The golden structures, for TestGolden and BenchSuite.
cc_library(
    name = "structures",
    testonly = True,
    srcs = ["cpp/Structures.cpp"],
    hdrs = ["cpp/Structures.h"],
    includes = ["cpp"],
    deps = [
        "//openroad/fsv:geometry",
        "//openroad/fsv:stack",
    ],
)

cc_test(
    name = "TestGolden",
    size = "large",
    srcs = ["cpp/TestGolden.cpp"],
    data = glob(["golden/*.txt"]),
    shard_count = 4,
    deps = [
        ":structures",
        "//openroad/fsv",
        "//openroad/fsv:geometry",
        "//openroad/fsv:stack",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestItf",
    srcs = ["cpp/TestItf.cpp"],
    data = [
        "GT2.itf",
        "sky130.itf",
    ],
    deps = [
        "//openroad/fsv:stack",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestSolver",
    size = "medium",
    srcs = ["cpp/TestSolver.cpp"],
    data = glob(["golden/*.txt"]),
    deps = [
        ":structures",
        "//openroad/fsv",
        "//openroad/fsv:core",
        "//openroad/fsv:geometry",
        "//openroad/fsv:stack",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_binary(
    name = "BenchCube",
    srcs = ["cpp/BenchCube.cpp"],
    deps = [
        "//openroad/fsv",
        "//openroad/fsv:geometry",
    ],
)

# The performance baseline; see README.md.
cc_binary(
    name = "BenchSuite",
    testonly = True,
    srcs = ["cpp/BenchSuite.cpp"],
    data = glob(["golden/*.txt"]) + [
        "field_solver_gcd.def",
        "sky130.itf",
        "sky130hd/sky130_fd_sc_hd_merged.lef",
        "sky130hd/sky130hd.tlef",
    ],
    deps = [
        ":structures",
        "//openroad/fsv",
        "//openroad/fsv:geometry",
        "//openroad/fsv:net_window",
        "//openroad/fsv:stack",
        "//openroad/odb/src/db",
        "//openroad/odb/src/defin",
        "//openroad/odb/src/lefin",
        "//openroad/utl",
    ],
)

# BenchSuite's cross sections on two threads, to keep it running.
cc_test(
    name = "BenchSuite_smoke",
    size = "small",
    srcs = ["cpp/BenchSuite.cpp"],
    args = [
        "--groups",
        "2d",
        "--threads",
        "2",
    ],
    data = glob(["golden/*.txt"]),
    deps = [
        ":structures",
        "//openroad/fsv",
        "//openroad/fsv:geometry",
        "//openroad/fsv:net_window",
        "//openroad/fsv:stack",
        "//openroad/odb/src/db",
        "//openroad/odb/src/defin",
        "//openroad/odb/src/lefin",
        "//openroad/utl",
    ],
)

cc_binary(
    name = "BenchGrid",
    srcs = ["cpp/BenchGrid.cpp"],
    deps = [
        "//openroad/fsv",
        "//openroad/fsv:geometry",
    ],
)

TESTS = [
    "field_solver_cross_section",
    "field_solver_errors",
    "field_solver_gcd",
    "field_solver_itf",
    "field_solver_nets",
    "field_solver_python",
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
        ] + ["sky130hd/**"],
    ) + [
        "sky130hd/sky130_fd_sc_hd_merged.lef",
        "sky130hd/sky130hd.tlef",
    ],
)

exports_files([
    "GT2.itf",
    "field_solver_gcd.def",
    "sky130.itf",
])

[regression_test(
    name = test_name,
    data = [":test_resources"],
) for test_name in TESTS]

man_tcl_check_test(
    name = "fsv_man_tcl_check",
    module = "fsv",
)

doc_check_test(
    name = "fsv_readme_msgs_check",
    data = [
        "//openroad/fsv:doc_files",
        "//openroad/fsv:messages_txt",
    ],
)
