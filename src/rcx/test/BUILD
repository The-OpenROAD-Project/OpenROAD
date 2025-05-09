# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2025, The OpenROAD Authors
load("//test:regression.bzl", "regression_test")

# From CMakeLists.txt or_integration_tests(TESTS
COMPULSORY_TESTS = [
    "45_gcd",
    "ext_pattern",
    "gcd",
    "generate_pattern",
    "names",
]

# Disabled in CMakeLists.txt
MANUAL_TESTS = [
    "generate_rules",
    "rcx_man_tcl_check",
    "rcx_readme_msgs_check",
]

ALL_TESTS = COMPULSORY_TESTS + MANUAL_TESTS

filegroup(
    name = "regression_resources",
    # Dependencies could be specified more narrowly per test case,
    # but at least it is not a glob of everything and there are
    # per test glob patterns below.
    srcs = [
        "45_patterns.rules",
        "45_via_resistance.tcl",
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
        "helpers.tcl",
        "patterns.rulesok",
        "rcx_unit_test.tcl",
        "sky130hs/sky130_fd_sc_hs__tt_025C_1v80.lib",
        "sky130hs/sky130_fd_sc_hs_merged.lef",
        "sky130hs/sky130hs.pdn.tcl",
        "sky130hs/sky130hs.rc",
        "sky130hs/sky130hs.rcx_rules",
        "sky130hs/sky130hs.tlef",
        "sky130hs/sky130hs.tracks",
        "sky130hs/sky130hs.vars",
        "sky130hs/sky130hs_ip_global.cfg",
        "sky130hs/sky130hs_std_cell.lef",
        "sky130hs/sky130hs_tt.lib",
    ],
)

[
    filegroup(
        name = test_name + "_resources",
        srcs = [":regression_resources"] + glob(
            [
                test_name + ".*",
            ],
        ) + {
            "gcd": ["ext_pattern.rules"],
            "ext_pattern": ["generate_pattern.defok"],
        }.get(test_name, []),
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
