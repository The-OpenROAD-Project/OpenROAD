# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("//test:regression.bzl", "regression_test")

package(features = ["layering_check"])

# These run a placer, a clock tree synthesizer or a router.  Pinning those
# tools' logs inside a watermarking golden would break this module whenever
# they change their output, so they assert their own numbers instead.
PASSFAIL_TESTS = [
    "cts_headroom",
    "numeric_options",
    "key_files",
    "cts_modes",
    "verify_nul",
    "strength_limits",
    "cts_protected",
    "cts_hierarchy",
    "cts_claim_names",
    "place_claim_names",
    "cts",
    "cts_drive",
    "cts_guard",
    "cts_endpoint_guard",
    "cts_clock_logic",
    "cts_liberty",
    "cts_clock_sets",
    "cts_skew_domains",
    "keygen",
    "place",
    "place_full_rollback",
    "place_guard",
    "place_keyed",
    "place_unconstrained",
    "python_api",
    "route_verify",
    "verify_malformed",
]

TESTS = [
    "bias",
    "bias_neutral",
    "cts_headroom",
    "numeric_options",
    "key_files",
    "cts_modes",
    "verify_nul",
    "strength_limits",
    "cts_protected",
    "cts_hierarchy",
    "cts_claim_names",
    "place_claim_names",
    "cts",
    "cts_drive",
    "cts_guard",
    "cts_endpoint_guard",
    "cts_clock_logic",
    "cts_liberty",
    "cts_clock_sets",
    "cts_skew_domains",
    "keygen",
    "place",
    "place_full_rollback",
    "place_guard",
    "place_keyed",
    "place_unconstrained",
    "place_unmarked",
    "python_api",
    "route_stat",
    "route_verify",
    "verify_malformed",
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

cc_test(
    name = "TestHmacSha256",
    srcs = ["cpp/TestHmacSha256.cpp"],
    deps = [
        "//src/wmk:hmac_sha256",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestClaims",
    srcs = ["cpp/TestClaims.cpp"],
    deps = [
        "//src/wmk:claims",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestTiming",
    srcs = ["cpp/TestTiming.cpp"],
    deps = [
        "//src/sta:opensta_lib",
        "//src/wmk:timing",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestClockTree",
    srcs = ["cpp/TestClockTree.cpp"],
    deps = [
        "//src/dbSta",
        "//src/dbSta:dbNetwork",
        "//src/odb/src/db",
        "//src/sta:opensta_lib",
        "//src/tst:nangate45_fixture",
        "//src/wmk:clock_tree",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
