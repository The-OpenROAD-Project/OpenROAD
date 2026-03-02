# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2025, The OpenROAD Authors
load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("//test:regression.bzl", "regression_test")

package(features = ["layering_check"])

TESTS = [
]

filegroup(
    name = "test_resources",
    # overly broad glob, could be refined later, but
    # symlinks are cheap and OpenROAD binary changes, the common
    # use case is that all tests have to be re-run.
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

cc_test(
    name = "tile_generator_test",
    srcs = ["cpp/TestTileGenerator.cpp"],
    features = ["-layering_check"],
    copts = ["-Isrc/web/src"],
    data = [
        "//test:nangate45_data",
    ],
    deps = [
        "//src/odb",
        "//src/tst",
        "//src/tst:nangate45_fixture",
        "//src/utl",
        "//src/web",
        "//third-party/lodepng",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
