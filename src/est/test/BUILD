# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2025, The OpenROAD Authors
load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("//test:regression.bzl", "regression_test")

package(features = ["layering_check"])

# From CMakeLists.txt or_integration_tests(TESTS
TESTS = [
    "make_parasitics1",
    "make_parasitics2",
    "make_parasitics3",
    "make_parasitics4",
    "make_parasitics5",
    "make_parasitics6",
    "make_parasitics7",
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
    name = "TestEstimateParasitics",
    srcs = ["cpp/TestEstimateParasitics.cc"],
    data = ["cpp/TestEstimateParasitics.v"],
    deps = [
        "//src/dbSta",
        "//src/dbSta:dbNetwork",
        "//src/est",
        "//src/odb/src/db",
        "//src/rsz",
        "//src/sta:opensta_lib",
        "//src/tst:integrated_fixture",
        "//src/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
