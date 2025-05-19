# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2025, The OpenROAD Authors
load("//test:regression.bzl", "regression_test")

# From CMakeLists.txt or_integration_tests(TESTS
COMPULSORY_TESTS = [
    "levelshifter",
    "write",
    "isolation",
    "isolation_select",
]

# Disabled in CMakeLists.txt
MANUAL_TESTS = [
    "upf_man_tcl_check",
    "upf_readme_msgs_check",
]

ALL_TESTS = COMPULSORY_TESTS + MANUAL_TESTS

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
