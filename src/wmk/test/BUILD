# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
load("//test:regression.bzl", "regression_test")

package(features = ["layering_check"])

TESTS = [
    "bias",
    "bias_neutral",
    "cts",
    "cts_guard",
    "place",
    "place_unmarked",
    "route_stat",
    "route_verify",
    "select",
    "strength",
    "verify",
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
