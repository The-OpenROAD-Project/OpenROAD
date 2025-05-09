# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2025, The OpenROAD Authors
load("//test:regression.bzl", "regression_test")

# From CMakeLists.txt or_integration_tests(TESTS
COMPULSORY_TESTS = [
    "aes_nangate45",
    "blif_reader",
    "blif_reader_const",
    "blif_reader_sequential",
    "blif_writer",
    "blif_writer_consts",
    "blif_writer_hanging",
    "blif_writer_input_output",
    "blif_writer_sequential",
    "const_cell_removal",
    "gcd_restructure",
    "aes_asap7",
    "gcd_asap7",
]

# Disabled in CMakeLists.txt
MANUAL_TESTS = [
    "rmp_man_tcl_check",
    "rmp_readme_msgs_check",
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
        "aes_nangate45.sdc",
        "aes_nangate45.tcl",
        "aes_nangate45.v",
        "design.def",
        "design_const.def",
        "design_hanging.def",
        "design_in_out.def",
        "design_sequential.def",
        "empty_cut_set.v",
        "gcd.sdc",
        "gcd_placed.def",
        "helpers.tcl",
        "rcon.def",
        "rcon.sdc",
        "side_outputs_extract.v",
        "side_outputs_extract_logic_depth.v",
        "simple_and_gate_extract.v",
        "sky130/sky130_fd_sc_hd__ss_n40C_1v40.lib",
        "sky130/sky130hd.tlef",
        "sky130/sky130hd_std_cell.lef",
        "sky130_const_cell.v",
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
