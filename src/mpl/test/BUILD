# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2025, The OpenROAD Authors
load("//test:regression.bzl", "regression_test")

# From CMakeLists.txt or_integration_tests(TESTS
COMPULSORY_TESTS = [
    "macro_only",
    "no_unfixed_macros",
    "guides1",
    "guides2",
    "io_constraints1",
    "io_constraints2",
    "io_pads1",
    "orientation_improve1",
    "orientation_improve2",
    "boundary_push1",
    "boundary_push2",
    "boundary_push3",
    "centralization1",
    "clocked_macro",
]

# Disabled in CMakeLists.txt
MANUAL_TESTS = [
    "mpl_man_tcl_check",
    "mpl_readme_msgs_check",
]

ALL_TESTS = COMPULSORY_TESTS + MANUAL_TESTS

filegroup(
    name = "regression_resources",
    # Dependencies could be specified more narrowly per test case,
    # but at least it is not a glob of everything and there are
    # per test glob patterns below.
    srcs = [
        "Nangate45/Nangate45.lef",
        "Nangate45/Nangate45.pdn.tcl",
        "Nangate45/Nangate45.rc",
        "Nangate45/Nangate45.rcx_rules",
        "Nangate45/Nangate45.tracks",
        "Nangate45/Nangate45.vars",
        "Nangate45/Nangate45_fast.lib",
        "Nangate45/Nangate45_lvt.lef",
        "Nangate45/Nangate45_lvt.lib",
        "Nangate45/Nangate45_slow.lib",
        "Nangate45/Nangate45_stdcell.lef",
        "Nangate45/Nangate45_tech.lef",
        "Nangate45/Nangate45_typ.lib",
        "Nangate45/fake_macros.lef",
        "Nangate45/fake_macros.lib",
        "Nangate45/fakeram45.cfg",
        "Nangate45/fakeram45_1024x32.lef",
        "Nangate45/fakeram45_1024x32.lib",
        "Nangate45/fakeram45_256x16.lef",
        "Nangate45/fakeram45_256x16.lib",
        "Nangate45/fakeram45_512x64.lef",
        "Nangate45/fakeram45_512x64.lib",
        "Nangate45/fakeram45_64x32.lef",
        "Nangate45/fakeram45_64x32.lib",
        "Nangate45/fakeram45_64x7.lef",
        "Nangate45/fakeram45_64x7.lib",
        "Nangate45/fakeram45_64x96.lef",
        "Nangate45/fakeram45_64x96.lib",
        "Nangate45/work_around_yosys/cells.v",
        "Nangate45_io/dummy_pads.lef",
        "Nangate45_io/dummy_pads.lib",
        "Nangate45_io/dummy_pads_m9.lef",
        "Nangate45_io/dummy_pads_stagger.lef",
        "helpers.tcl",
        "testcases/boundary_push1.def",
        "testcases/boundary_push2.def",
        "testcases/centralization1.def",
        "testcases/clocked_macro.def",
        "testcases/clocked_macro.lef",
        "testcases/clocked_macro.lib",
        "testcases/clocked_macro.v",
        "testcases/guides1.def",
        "testcases/guides1.v",
        "testcases/io_constraints1.def",
        "testcases/io_constraints1.v",
        "testcases/io_pads1.def",
        "testcases/macro_only.def",
        "testcases/macro_only.lef",
        "testcases/macro_only.lib",
        "testcases/macro_only.v",
        "testcases/no_unfixed_macros.def",
        "testcases/orientation_improve1.def",
        "testcases/orientation_improve1.lef",
    ],
)

[
    filegroup(
        name = test_name + "_resources",
        data = [":regression_resources"] + glob(
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
