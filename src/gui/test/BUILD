# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2025, The OpenROAD Authors
load("//test:regression.bzl", "regression_test")

package(features = ["layering_check"])

# From CMakeLists.txt or_integration_tests(TESTS
ALWAYS_TESTS = [
    "supported",
]

CONDITIONAL_TESTS = [
    "screenshot",
]

ALL_TESTS = ALWAYS_TESTS + CONDITIONAL_TESTS

filegroup(
    name = "regression_resources",
    # Dependencies could be specified more narrowly per test case,
    # but at least it is not a glob of everything and there are
    # per test glob patterns below.
    srcs = [
        "Nangate45/Nangate45_stdcell.lef",
        "Nangate45/Nangate45_tech.lef",
        "helpers.tcl",
        "nangate_gcd/floorplan.def",
    ],
)

[
    filegroup(
        name = test_name + "_resources",
        srcs = [":regression_resources"] + glob(
            [
                test_name + ".*",
            ],
        ),
    )
    for test_name in ALL_TESTS
]

[
    regression_test(
        name = test_name,
        data = [":" + test_name + "_resources"],
        visibility = ["//visibility:public"],
    )
    for test_name in ALWAYS_TESTS
]

[
    regression_test(
        name = test_name,
        data = [":" + test_name + "_resources"],
        target_compatible_with = select(
            {
                "//:platform_gui": [],
                "//conditions:default": ["@platforms//:incompatible"],
            },
        ),
        visibility = ["//visibility:public"],
    )
    for test_name in CONDITIONAL_TESTS
]
