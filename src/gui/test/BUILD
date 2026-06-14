load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("@rules_python//python:defs.bzl", "py_test")

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2025, The OpenROAD Authors
load("//test:regression.bzl", "doc_check_test", "regression_test")

package(features = ["layering_check"])

# From CMakeLists.txt or_integration_tests(TESTS
ALL_TESTS = [
    "supported",
]

# dump_heatmap_headless requires the full Qt GUI implementation.  The default
# Bazel openroad binary links the stub GUI, so the equivalent Bazel regression is
# the C++ heatmap_sync_test against //src/gui:gui_qt_headless below.

filegroup(
    name = "test_resources",
    # overly broad glob, could be refined later, but
    # symlinks are cheap and OpenROAD binary changes, the common
    # use case is that all tests have to be re-run.
    srcs = glob(
        ["**/*"],
        exclude = [
            test + "." + ext
            for test in ALL_TESTS
            for ext in [
                "tcl",
                "py",
            ]
        ],
    ),
)

[
    regression_test(
        name = test_name,
        data = [":test_resources"],
        visibility = ["//visibility:public"],
    )
    for test_name in ALL_TESTS
]

py_test(
    name = "gui_man_tcl_check",
    srcs = ["gui_man_tcl_check.py"],
    data = ["//src/gui:doc_files"],
    tags = ["doc_check"],
    deps = ["//docs/src/scripts:extract_utils"],
)

doc_check_test(
    name = "gui_readme_msgs_check",
    data = [
        "//src/gui:doc_files",
        "//src/gui:messages_txt",
    ],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "heatmap_sync_test",
    srcs = ["heatmap_sync_test.cpp"],
    deps = [
        "//src/gui:gui_qt_headless",
        "//src/odb",
        "//src/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
