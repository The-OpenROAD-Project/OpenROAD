# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2025, The OpenROAD Authors
load("//test:regression.bzl", "regression_test")

# From CMakeLists.txt or_integration_tests(TESTS
COMPULSORY_TESTS = [
    "block_sta1",
    "clock_pin",
    "constant1",
    "find_clks1",
    "find_clks2",
    "hier2",
    "hierclock",
    "hierwrite",
    "make_port",
    "network_edit1",
    "power1",
    "read_liberty1",
    "read_verilog1",
    "read_verilog2",
    "read_verilog3",
    "read_verilog4",
    "read_verilog5",
    "read_verilog6",
    "read_verilog7",
    "read_verilog8",
    "read_verilog9",
    "read_verilog10",
    "read_verilog11",
    "readdb_hier",
    "report_cell_usage",
    "report_cell_usage_file",
    "report_cell_usage_modinsts",
    "report_cell_usage_modinsts_metrics",
    "report_cell_usage_physical_only",
    "report_json1",
    "report_timing_histogram",
    "sdc_get1",
    "sdc_names1",
    "sdc_names2",
    "sta1",
    "sta2",
    "sta3",
    "sta4",
    "sta5",
    "write_sdc1",
    "write_verilog1",
    "write_verilog2",
    "write_verilog3",
    "write_verilog4",
    "write_verilog5",
    "write_verilog6",
    "write_verilog7",
    "write_verilog8",
]

ALL_TESTS = COMPULSORY_TESTS

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
        "bidir1.lef",
        "bidir1.lib",
        "bus1.def",
        "bus1.lef",
        "bus1.lib",
        "bus1.v",
        "dont_touch_attr.tcl",
        "dont_touch_attr.v",
        "example1.def",
        "example1.dspef",
        "example1.lef",
        "example1.sdf",
        "example1.v",
        "example1_fast.lib",
        "example1_slow.lib",
        "example1_typ.lib",
        "helpers.tcl",
        "hier1.def",
        "hier1.sdc",
        "hier1.v",
        "hier2_out.vok",
        "hierclock_gate.v",
        "hierclock_out.vok",
        "liberty1.lef",
        "liberty1.lib",
        "pad.lef",
        "pad.lib",
        "reg.txt",
        "reg1.def",
        "reg1.v",
        "reg3.def",
        "reg4.def",
        "reg5.v",
        "reg6.def",
        "report_cell_usage_no_taps.def",
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
            "report_cell_usage_file": [
                "report_cell_usage.def",
            ],
            "write_verilog6": [
                "write_verilog5.def",
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
