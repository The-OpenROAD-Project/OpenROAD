# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2025, The OpenROAD Authors

load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("//test:regression.bzl", "regression_test")

package(features = ["layering_check"])

# From CMakeLists.txt or_integration_tests(TESTS
ALL_TESTS = [
    "block_sta1",
    "check_axioms",
    "clock_pin",
    "constant1",
    "escape_slash",
    "escape_slash_hier",
    "find_clks1",
    "find_clks2",
    "get_ports1",
    "get_ports1_hier",
    "hier2",
    "hier3",
    "hierclock",
    "hierwrite",
    "hier_deep",
    "make_port",
    "network_edit1",
    "power1",
    "read_liberty1",
    "read_vcd",
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
    "read_verilog10_no_prop",
    "read_verilog11",
    "readdb_hier",
    "report_cell_usage",
    "report_cell_usage_file",
    "report_cell_usage_modinsts",
    "report_cell_usage_modinsts_metrics",
    "report_cell_usage_physical_only",
    "report_json1",
    "report_timing_histogram",
    "report_logic_depth_histogram",
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
    "write_verilog9",
    "write_verilog9_hier",
]

filegroup(
    name = "regression_resources",
    srcs = [
        "Nangate45/Nangate45.lef",
        "Nangate45/Nangate45.pdn.tcl",
        "Nangate45/Nangate45.rc",
        "Nangate45/Nangate45.tracks",
        "Nangate45/Nangate45.vars",
        "Nangate45/Nangate45_fast.lib",
        "Nangate45/Nangate45_slow.lib",
        "Nangate45/Nangate45_stdcell.lef",
        "Nangate45/Nangate45_tech.lef",
        "Nangate45/Nangate45_typ.lib",
        "helpers.tcl",
        "liberty1.lef",
        "liberty1.lib",
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
            "block_sta1": [
                "example1_typ.lib",
                "example1.def",
                "example1.lef",
            ],
            "find_clks1": [
                "pad.lef",
                "pad.lib",
            ],
            "get_ports1_hier": [
                "get_ports1.v",
            ],
            "hier2": [
                "example1_typ.lib",
                "example1.lef",
                "hier2_out.vok",
            ],
            "hier3": [
                "example1_typ.lib",
                "example1.lef",
            ],
            "hier_deep": [
                "example1_typ.lib",
                "example1.lef",
            ],
            "hierclock": [
                "hierclock_gate.v",
                "hierclock_out.vok",
            ],
            "network_edit1": [
                "reg3.def",
            ],
            "read_vcd": glob(
                [
                    "Element.*",
                    "MockArray.*",
                    "asap7/*",
                ],
            ),
            "read_verilog1": [
                "reg1.v",
            ],
            "read_verilog10": [
                "hier1.v",
            ],
            "read_verilog10_no_prop": [
                "hier1.v",
            ],
            "read_verilog11": [
                "hier1.v",
            ],
            "read_verilog2": [
                "hier1.v",
            ],
            "read_verilog3": [
                "bus1.lef",
                "bus1.lib",
                "bus1.v",
            ],
            "read_verilog5": [
                "bidir1.lef",
                "bidir1.lib",
            ],
            "read_verilog9": [
                "reg1.v",
            ],
            "readdb_hier": [
                "hier1.v",
            ],
            "report_cell_usage_file": [
                "report_cell_usage.def",
            ],
            "report_cell_usage_modinsts": [
                "hier1.v",
            ],
            "report_cell_usage_modinsts_metrics": [
                "hier1.v",
            ],
            "report_cell_usage_physical_only": [
                "report_cell_usage_no_taps.def",
            ],
            "report_json1": [
                "reg6.def",
            ],
            "report_timing_histogram": [
                "example1_slow.lib",
                "example1.def",
                "example1.lef",
            ],
            "sdc_get1": [
                "example1_slow.lib",
                "example1.def",
                "example1.lef",
            ],
            "sdc_names1": [
                "hier1.def",
            ],
            "sta1": [
                "example1_slow.lib",
                "example1.def",
                "example1.lef",
            ],
            "sta2": [
                "example1_slow.lib",
                "example1.def",
                "example1.lef",
                "example1.sdf",
                "example1.v",
            ],
            "sta3": [
                "example1_slow.lib",
                "example1_fast.lib",
                "example1.def",
                "example1.lef",
            ],
            "sta4": [
                "example1_slow.lib",
                "example1.dspef",
                "example1.def",
                "example1.lef",
            ],
            "sta5": [
                "example1_fast.lib",
                "example1_slow.lib",
                "example1_typ.lib",
                "example1.def",
                "example1.lef",
            ],
            "write_sdc1": [
                "hier1.sdc",
                "hier1.v",
            ],
            "write_verilog1": [
                "reg1.def",
            ],
            "write_verilog2": [
                "reg4.def",
            ],
            "write_verilog3": [
                "bus1.def",
                "bus1.lef",
                "bus1.lib",
            ],
            "write_verilog6": [
                "write_verilog5.def",
            ],
            "write_verilog8": [
                "reg5.v",
            ],
            "write_verilog9_hier": [
                "write_verilog9.v",
            ],
        }.get(test_name, []),
    )
    for test_name in ALL_TESTS
]

[
    regression_test(
        name = test_name,
        data = [":" + test_name + "_resources"],
        visibility = ["//visibility:public"],
    )
    for test_name in ALL_TESTS
]

cc_test(
    name = "dbsta_hconn_unittest",
    srcs = ["cpp/TestHconn.cpp"],
    data = [
        "Nangate45/Nangate45.lef",
        "Nangate45/Nangate45_typ.lib",
    ],
    deps = [
        "//src/dbSta",
        "//src/dbSta:dbNetwork",
        "//src/odb",
        "//src/sta:opensta_lib",
        "//src/tst",
        "//src/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "dbsta_unittest",
    srcs = ["cpp/TestDbSta.cc"],
    data = [
        "Nangate45/Nangate45.lef",
        "Nangate45/Nangate45_typ.lib",
        "cpp/TestDbSta_0.v",
    ],
    deps = [
        "//src/dbSta",
        "//src/dbSta:dbNetwork",
        "//src/odb",
        "//src/sta:opensta_lib",
        "//src/tst:integrated_fixture",
        "//src/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_read_verilog",
    srcs = ["cpp/TestReadVerilog.cpp"],
    data = glob(["cpp/TestReadVerilog*.v"]),
    deps = [
        "//src/dbSta",
        "//src/dbSta:dbNetwork",
        "//src/dbSta:dbReadVerilog",
        "//src/odb",
        "//src/sta:opensta_lib",
        "//src/tst:integrated_fixture",
        "//src/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
