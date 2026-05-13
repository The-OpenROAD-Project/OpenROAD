# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2025, The OpenROAD Authors
load("//test:regression.bzl", "regression_test")

package(features = ["layering_check"])

# From CMakeLists.txt or_integration_tests(TESTS
COMPULSORY_TESTS = [
    "make_8x8_sky130",
    "make_7x7_nangate45",
]

PASSFAIL_TESTS = [
]

ALL_TESTS = COMPULSORY_TESTS + PASSFAIL_TESTS

filegroup(
    name = "regression_resources",
    # Dependencies could be specified more narrowly per test case,
    # but at least it is not a glob of everything and there are
    # per test glob patterns below.
    srcs = [
        "helpers.tcl",
        "sky130hd/sky130_fd_sc_hd__ff_n40C_1v95.lib",
        "sky130hd/sky130_fd_sc_hd__ss_n40C_1v40.lib",
        "sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib",
        "sky130hd/sky130_fd_sc_hd_merged.lef",
        "sky130hd/sky130hd.pdn.tcl",
        "sky130hd/sky130hd.rc",
        "sky130hd/sky130hd.rcx_rules",
        "sky130hd/sky130hd.tlef",
        "sky130hd/sky130hd.tracks",
        "sky130hd/sky130hd.vars",
        "sky130hd/sky130hd_multi_patterned.tlef",
        "sky130hd/sky130hd_std_cell.lef",
        "sky130hd/sky130hd_std_cell_vt.lef",
        "sky130hd/sky130hd_tt.lib",
        "sky130hd/sky130hd_vt.tlef",
        "sky130hd/work_around_yosys/formal_pdk.v",
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
            "make_7x7_nangate45": [
                "make_7x7_nangate45_behavioral.vok",
                "Nangate45/Nangate45.lef",
                "Nangate45/Nangate45_tech.lef",
                "Nangate45/Nangate45_typ.lib",
            ],
            "make_8x8_sky130": ["make_8x8_sky130_behavioral.vok"],
        }.get(test_name, []),
    )
    for test_name in ALL_TESTS
]

[
    regression_test(
        name = test_name,
        size = "large",
        check_log = False if test_name in PASSFAIL_TESTS else True,
        check_passfail = True if test_name in PASSFAIL_TESTS else False,
        data = [":" + test_name + "_resources"],
        visibility = ["//visibility:public"],
    )
    for test_name in ALL_TESTS
]
