# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("@rules_python//python:defs.bzl", "py_test")
load("//test:regression.bzl", "doc_check_test", "regression_test")

package(features = ["layering_check"])

# These run a placer, a clock tree synthesizer or a router.  Pinning those
# tools' logs inside a watermarking golden would break this module whenever
# they change their output, so they assert their own numbers instead.
PASSFAIL_TESTS = [
    "verify_evidence",
    "verify_carriers",
    "claim_publication",
    "cts_empty_claims",
    "cts_headroom",
    "cts_fanout",
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
    "routing_calibration",
    "routing_defaults",
    "routing_lifecycle",
    "routing_report",
    "routing_ties",
    "verify_malformed",
    "wrong_key",
    "three_stages",
]

TESTS = [
    "verify_evidence",
    "verify_carriers",
    "claim_publication",
    "cts_empty_claims",
    "cts_headroom",
    "cts_fanout",
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
    "routing_calibration",
    "routing_defaults",
    "routing_lifecycle",
    "routing_report",
    "routing_ties",
    "verify_malformed",
    "select",
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

py_test(
    name = "wmk_man_tcl_check",
    srcs = ["wmk_man_tcl_check.py"],
    data = ["//src/wmk:doc_files"],
    tags = ["doc_check"],
    deps = ["//docs/src/scripts:extract_utils"],
)

doc_check_test(
    name = "wmk_readme_msgs_check",
    data = [
        "//src/wmk:doc_files",
        "//src/wmk:messages_txt",
    ],
    visibility = ["//visibility:public"],
)

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
    name = "TestMarks",
    srcs = ["cpp/TestMarks.cpp"],
    deps = [
        "//src/wmk:marks",
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
    name = "TestDriverHeadroom",
    srcs = ["cpp/TestDriverHeadroom.cpp"],
    deps = [
        "//src/dbSta",
        "//src/dbSta:dbNetwork",
        "//src/odb/src/db",
        "//src/sta:opensta_lib",
        "//src/tst:nangate45_fixture",
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

cc_test(
    name = "TestClaimFile",
    srcs = ["cpp/TestClaimFile.cpp"],
    deps = [
        "//src/wmk:claim_file",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestRoutingStat",
    srcs = ["cpp/TestRoutingStat.cpp"],
    deps = [
        "//src/wmk",
        "//src/wmk:wmk_private_hdrs",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestVerifyResult",
    srcs = ["cpp/TestVerifyResult.cpp"],
    deps = [
        "//src/wmk:verify_result",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
