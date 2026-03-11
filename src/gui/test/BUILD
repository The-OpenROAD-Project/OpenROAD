# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2025, The OpenROAD Authors
load("//test:regression.bzl", "doc_check_test", "regression_test")

package(features = ["layering_check"])

# From CMakeLists.txt or_integration_tests(TESTS
ALL_TESTS = [
    "supported",
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

[
    doc_check_test(
        name = test_name,
        data = [
            "//src/gui:README.md",
            "//src/gui:messages_txt",
        ] + (["//src/gui:src/gui.tcl"] if "man_tcl" in test_name else []),
        visibility = ["//visibility:public"],
    )
    for test_name in [
        "gui_man_tcl_check",
        "gui_readme_msgs_check",
    ]
]
