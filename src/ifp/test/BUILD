# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2025, The OpenROAD Authors
load("//test:regression.bzl", "regression_test")

package(features = ["layering_check"])

# From CMakeLists.txt or_integration_tests(TESTS
COMPULSORY_TESTS = [
    "hybrid_rows",
    "hybrid_rows2",
    "init_floorplan1",
    "init_floorplan2",
    "init_floorplan3",
    "init_floorplan4",
    "init_floorplan5",
    "init_floorplan6",
    "init_floorplan7",
    "init_floorplan8",
    "init_floorplan9",
    "init_floorplan10",
    "init_floorplan11",
    "init_floorplan12",
    "init_floorplan_dbl_row",
    "init_floorplan_even_rows",
    "init_floorplan_flip_sites",
    "init_floorplan_gap",
    "init_floorplan_odd_rows",
    "init_floorplan_polygon",
    "init_floorplan_polygon2",
    "make_polygon_rows1",
    "make_polygon_rows2",
    "make_polygon_rows3",
    "make_polygon_rows4",
    "make_rows",
    "make_tracks1",
    "make_tracks2",
    "make_tracks3",
    "make_tracks4",
    "make_tracks5",
    "make_tracks6",
    "make_tracks7",
    "multi_height1",
    "multi_height2",
    "placement_blockage1",
    "placement_blockage2",
    "tiecells",
    "upf_shifter_test",
    "upf_test",
]

# Disabled in CMakeLists.txt
MANUAL_TESTS = [
    "ifp_man_tcl_check",
    "ifp_readme_msgs_check",
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
        "Nangate45_data/macros.v",
        "asap7_data/asap7.def",
        "asap7_data/asap7.lef",
        "helpers.tcl",
        "make_rows_no_rows.def",
        "reg1.def",
        "reg1.v",
        "reg2.v",
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
        "upf/mpd_shifter.upf",
        "upf/mpd_top.upf",
        "upf/mpd_top.v",
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
