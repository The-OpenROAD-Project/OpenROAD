# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
load("//test:regression.bzl", "regression_test")

package(features = ["layering_check"])

# These run a placer, a clock tree synthesizer or a router.  Pinning those
# tools' logs inside a watermarking golden would break this module whenever
# they change their output, so they assert their own numbers instead.
PASSFAIL_TESTS = [
    "cts",
    "cts_drive",
    "cts_guard",
    "keygen",
    "place",
    "place_guard",
    "place_keyed",
    "python_api",
    "route_verify",
]

TESTS = [
    "bias",
    "bias_neutral",
    "cts",
    "cts_drive",
    "cts_guard",
    "keygen",
    "place",
    "place_guard",
    "place_keyed",
    "place_unmarked",
    "python_api",
    "route_stat",
    "route_verify",
    "select",
    "strength",
    "verify",
    "via_layer",
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
    check_log = False if test_name in PASSFAIL_TESTS else True,
    check_passfail = True if test_name in PASSFAIL_TESTS else False,
    data = [":test_resources"],
) for test_name in TESTS]
