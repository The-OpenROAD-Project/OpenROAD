# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2025, The OpenROAD Authors
load("//test:regression.bzl", "regression_test")

# From CMakeLists.txt or_integration_tests(TESTS
COMPULSORY_TESTS = [
    "array",
    "array_ins_delay",
    "array_no_blockages",
    "balance_levels",
    "check_buffers",
    "check_buffers_blockages",
    "check_buffers_blockages_merge",
    "check_charBuf",
    "check_max_fanout1",
    "check_max_fanout2",
    "check_wire_rc_cts",
    "dummy_load",
    "find_clock",
    "find_clock_pad",
    "insertion_delay",
    "lvt_lib",
    "max_cap",
    "no_clocks",
    "no_sinks",
    "post_cts_opt",
    "simple_test",
    "simple_test_clustered",
    "simple_test_clustered_max_cap",
    "simple_test_hier",
    "twice",
]

# Disabled in CMakeLists.txt
MANUAL_TESTS = [
    "cts_man_tcl_check",
    "cts_readme_msgs_check",
]

ALL_TESTS = COMPULSORY_TESTS + MANUAL_TESTS

filegroup(
    name = "regression_resources",
    # Dependencies could be specified more narrowly per test case,
    # but at least it is not a glob of everything and there are
    # per test glob patterns below.
    srcs = [
        "16sinks.def",
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
        "array_dummy.tcl",
        "array_full_flow.tcl",
        "array_tile.lef",
        "array_tile.lib",
        "array_tile_ins_delay.lef",
        "array_tile_ins_delay.lib",
        "cts-helpers.tcl",
        "helpers.tcl",
        "ihp-sg13g2_data/IOLib.lef",
        "ihp-sg13g2_data/IOLib_dummy.lib",
        "ihp-sg13g2_data/setRC.tcl",
        "ihp-sg13g2_data/sg13g2_stdcell.lef",
        "ihp-sg13g2_data/sg13g2_stdcell_typ_1p20V_25C.lib",
        "ihp-sg13g2_data/sg13g2_tech.lef",
        "no_clock.def",
        "pad.lef",
        "pad.lib",
        "simple_test_hier_out.vok",
        "simple_test_out.defok",
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
        data = [":regression_resources"] + glob(
            [
                test_name + ".*",
            ],
        ) + {
            "check_max_fanout2": [
                "check_buffers.def",
            ],
            "check_wire_rc_cts": [
                "check_buffers.def",
            ],
            "dummy_load": [
                "check_buffers.def",
            ],
            "max_cap": [
                "//src/rsz/test:hi_fanout.tcl",
            ],
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
