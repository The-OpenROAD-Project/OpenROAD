# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2026, The OpenROAD Authors

load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("//test:regression.bzl", "regression_test")

package(features = ["layering_check"])

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
    "memory_nangate45",
    "aes_dontuse_nangate45",
    "aes_donttouch_nangate45",
    "aes_annealing",
    "gcd_annealing1",
    "gcd_annealing2",
    "aes_genetic",
    "gcd_genetic",
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
        "aes_asap7.sdc",
        "aes_asap7.v",
        "aes_nangate45.sdc",
        "aes_nangate45.tcl",
        "aes_nangate45.v",
        "asap7/asap7_tech_1x_201209.lef",
        "asap7/asap7sc7p5t_28_R_1x_220121a.lef",
        "asap7/asap7sc7p5t_AO_RVT_FF_nldm_211120.lib.gz",
        "asap7/asap7sc7p5t_AO_RVT_SS_nldm_211120.lib.gz",
        "asap7/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz",
        "asap7/asap7sc7p5t_INVBUF_RVT_SS_nldm_220122.lib.gz",
        "asap7/asap7sc7p5t_OA_RVT_FF_nldm_211120.lib.gz",
        "asap7/asap7sc7p5t_OA_RVT_SS_nldm_211120.lib.gz",
        "asap7/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib",
        "asap7/asap7sc7p5t_SEQ_RVT_SS_nldm_220123.lib",
        "asap7/asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib.gz",
        "asap7/asap7sc7p5t_SIMPLE_RVT_SS_nldm_211120.lib.gz",
        "design.def",
        "design_const.def",
        "design_hanging.def",
        "design_in_out.def",
        "design_sequential.def",
        "gcd.sdc",
        "gcd_asap7.sdc",
        "gcd_asap7.v",
        "gcd_placed.def",
        "helpers.tcl",
        "rcon.def",
        "rcon.sdc",
        "sky130/sky130_fd_sc_hd__ss_n40C_1v40.lib",
        "sky130/sky130hd.tlef",
        "sky130/sky130hd_std_cell.lef",
    ],
)

[
    filegroup(
        name = test_name + "_resources",
        srcs = [":regression_resources"] + glob(
            [
                test_name + ".*",
            ],
        ) + ([
            "asap7/setRC.tcl",
        ] if "asap7" in test_name else []),
    )
    for test_name in ALL_TESTS
]

[
    regression_test(
        name = test_name,
        data = [":" + test_name + "_resources"],
        tags = ["manual"] if test_name in MANUAL_TESTS else [],
        visibility = ["//visibility:public"],
    )
    for test_name in ALL_TESTS
]

cc_test(
    name = "rmp_abc_unittest",
    srcs = [
        "cpp/TestAbc.cc",
    ],
    data = [
        "Nangate45/Nangate45_stdcell.lef",
        "Nangate45/Nangate45_tech.lef",
        "Nangate45/Nangate45_typ.lib",
        "aes_nangate45.v",
    ],
    features = ["-layering_check"],  # TODO: includes private headers
    includes = [
        "../src",
    ],
    deps = [
        "//src/cut",
        "//src/dbSta",
        "//src/dbSta:dbReadVerilog",
        "//src/odb",
        "//src/rmp",
        "//src/sta:opensta_lib",
        "//src/tst",
        "//src/utl",
        "@abc",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
        "@tcl_lang//:tcl",
    ],
)
