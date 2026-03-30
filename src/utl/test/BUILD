load("@rules_python//python:defs.bzl", "py_test")
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2025, The OpenROAD Authors

# Test Parity Note: All tests from CMakeLists.txt are included in this BUILD file.
# No tests are intentionally excluded. See .kiro/specs/bazel-cmake-test-parity/intentional_exclusions.md

load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("//test:regression.bzl", "doc_check_test", "regression_test")

package(features = ["layering_check"])

# From CMakeLists.txt or_integration_tests(TESTS
COMPULSORY_TESTS = [
    "logger_max_messages",
    "logger_redirection",
    "logger_redirection_nonewline",
    "tee",
    "tee_fails",
    "test_error",
    "test_error_exception",
    "test_info",
    "test_metrics",
    "test_suppress_message",
]

# From CMakeLists.txt or_integration_tests(PASSFAIL_TESTS
PASSFAIL_TESTS = [
    # NOTE: cpp_tests excluded - it's a CMake-specific wrapper script that expects
    # build directory structure not compatible with Bazel
    # "cpp_tests",
]

ALL_TESTS = COMPULSORY_TESTS + PASSFAIL_TESTS

filegroup(
    name = "regression_resources",
    # Dependencies could be specified more narrowly per test case,
    # but at least it is not a glob of everything and there are
    # per test glob patterns below.
    srcs = [
        "helpers.tcl",
        "logger_redirection_1.rptok",
        "logger_redirection_2.rptok",
        "logger_redirection_3.rptok",
        "logger_redirection_4.rptok",
        "logger_redirection_nonewline_1.rptok",
        "logger_redirection_nonewline_2.rptok",
        "metrics.jsonok",
        "tee_1.rptok",
        "tee_2.rptok",
        "tee_3.rptok",
        "tee_4.rptok",
        "tee_5.rptok",
        "tee_6.rptok",
        "tee_7.rptok",
        "tee_8.rptok",
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
        check_passfail = True if test_name in PASSFAIL_TESTS else False,
        data = [":" + test_name + "_resources"],
        tags = [] if test_name in COMPULSORY_TESTS else ["manual"],
        visibility = ["//visibility:public"],
    )
    for test_name in ALL_TESTS
]

cc_test(
    name = "utl_cfile_unittest",
    srcs = ["cpp/TestCFileUtils.cpp"],
    deps = [
        "//src/utl",
        "@boost.asio",
        "@boost.beast",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestCFileUtils",
    srcs = ["cpp/TestCFileUtils.cpp"],
    deps = [
        "//src/utl",
        "@boost.asio",
        "@boost.beast",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestMetrics",
    srcs = [
        "cpp/TestMetrics.cpp",
    ],
    deps = [
        "//src/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestAlgorithms",
    srcs = [
        "cpp/TestAlgorithms.cpp",
    ],
    deps = [
        "//src/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestSuppressStdout",
    srcs = ["cpp/TestSuppressStdout.cpp"],
    deps = [
        "//src/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

py_test(
    name = "utl_man_tcl_check",
    srcs = ["utl_man_tcl_check.py"],
    data = ["//src/utl:doc_files"],
    tags = ["doc_check"],
    deps = ["//docs/src/scripts:extract_utils"],
)

doc_check_test(
    name = "utl_readme_msgs_check",
    data = [
        "//src/utl:doc_files",
        "//src/utl:messages_txt",
    ],
    visibility = ["//visibility:public"],
)
