# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("@rules_python//python:defs.bzl", "py_test")
load("//bazel:readme_msgs_checker.bzl", "readme_msgs_check_test")
load("//test:regression.bzl", "regression_test")

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
    "key_file_format",
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

TESTS = PASSFAIL_TESTS + [
    "place_unmarked",
    "route_stat",
    "select",
    "verify",
    "via_layer",
]

filegroup(
    name = "test_resources",
    srcs = glob(
        ["**/*"],
        # Local test outputs must not become Bazel inputs.
        exclude = [
            "**/__pycache__/**",
            "results/**",
        ] + [
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
    data = ["//openroad/wmk:doc_files"],
    tags = ["doc_check"],
    deps = ["//docs/src/scripts:extract_utils"],
)

readme_msgs_check_test(
    name = "wmk_readme_msgs_check",
    data = [
        "//openroad/wmk:doc_files",
        "//openroad/wmk:messages_txt",
    ],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "TestHmacSha256",
    srcs = ["cpp/TestHmacSha256.cpp"],
    deps = [
        "//openroad/wmk:hmac_sha256",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestMarks",
    srcs = ["cpp/TestMarks.cpp"],
    deps = [
        "//openroad/wmk:marks",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestClaims",
    srcs = ["cpp/TestClaims.cpp"],
    deps = [
        "//openroad/wmk:claims",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestTiming",
    srcs = ["cpp/TestTiming.cpp"],
    deps = [
        "//openroad/sta:opensta_lib",
        "//openroad/wmk:timing",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestDriverHeadroom",
    srcs = ["cpp/TestDriverHeadroom.cpp"],
    deps = [
        "//openroad/dbSta",
        "//openroad/dbSta:dbNetwork",
        "//openroad/odb/src/db",
        "//openroad/sta:opensta_lib",
        "//openroad/tst:nangate45_fixture",
        "//openroad/wmk:timing",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestClockTree",
    srcs = ["cpp/TestClockTree.cpp"],
    deps = [
        "//openroad/dbSta",
        "//openroad/dbSta:dbNetwork",
        "//openroad/odb/src/db",
        "//openroad/sta:opensta_lib",
        "//openroad/tst:nangate45_fixture",
        "//openroad/wmk:clock_tree",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestClaimFile",
    srcs = ["cpp/TestClaimFile.cpp"],
    deps = [
        "//openroad/wmk:claim_file",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestRoutingStat",
    srcs = ["cpp/TestRoutingStat.cpp"],
    deps = [
        "//openroad/wmk:wmk_impl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestVerifyResult",
    srcs = ["cpp/TestVerifyResult.cpp"],
    deps = [
        "//openroad/wmk:verify_result",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
