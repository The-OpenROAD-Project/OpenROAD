load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("//bazel:man_tcl_checker.bzl", "man_tcl_check_test")

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2025, The OpenROAD Authors
load("//test:regression.bzl", "doc_check_test", "regression_test")

package(features = ["layering_check"])

COMPULSORY_TESTS = [
    "levelshifter",
    "write",
    "isolation",
    "isolation_select",
]

ALL_TESTS = COMPULSORY_TESTS

filegroup(
    name = "regression_resources",
    # Dependencies could be specified more narrowly per test case,
    # but at least it is not a glob of everything and there are
    # per test glob patterns below.
    srcs = [
        "data/isolation/mpd_top.upf",
        "data/isolation/mpd_top.v",
        "data/isolation/mpd_top_select_iso.upf",
        "data/mpd_top/mpd_top.upf",
        "data/mpd_top/mpd_top.v",
        "data/mpd_top/mpd_top_combined.upf",
        "data/mpd_top/mpd_top_ls.upf",
        "data/sky130hd/sky130_fd_sc_hd.tlef",
        "data/sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib",
        "data/sky130hd/sky130_fd_sc_hd_merged.lef",
        "helpers.tcl",
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
        tags = [] if test_name in COMPULSORY_TESTS else ["manual"],
        visibility = ["//visibility:public"],
    )
    for test_name in ALL_TESTS
]

regression_test(
    name = "read_upf_errors",
    check_log = False,
    check_passfail = True,
    data = [":regression_resources"],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "TestPowerSwitch",
    size = "small",
    srcs = ["cpp/TestPowerSwitch.cc"],
    deps = [
        "//openroad/odb/src/db",
        "//openroad/tst:db_fixture",
        "//openroad/upf",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestPowerDomainRegions",
    size = "small",
    srcs = ["cpp/TestPowerDomainRegions.cc"],
    deps = [
        "//openroad/dbSta",
        "//openroad/odb/src/db",
        "//openroad/tst",
        "//openroad/tst:nangate45_fixture",
        "//openroad/upf",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

man_tcl_check_test(
    name = "upf_man_tcl_check",
    module = "upf",
)

doc_check_test(
    name = "upf_readme_msgs_check",
    data = [
        "//openroad/upf:doc_files",
        "//openroad/upf:messages_txt",
    ],
    visibility = ["//visibility:public"],
)
