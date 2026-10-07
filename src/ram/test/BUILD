# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2025, The OpenROAD Authors
load("//test:regression.bzl", "regression_test")

package(features = ["layering_check"])

# From CMakeLists.txt or_integration_tests(TESTS
COMPULSORY_TESTS = [
    "make_7x7_2r1w_nangate45",
    "make_7x7_nangate45",
    "make_8x8_2r1w_sky130",
    "make_8x8_sky130",
]

PASSFAIL_TESTS = [
    "generate_regfile_asap7",
    "generate_regfile_clock_gate_asap7",
    "generate_regfile_reset_asap7",
    "generate_regfile_reset_high_asap7",
    "generate_regfile_sv_ports_asap7",
    "generate_regfile_write_priority_asap7",
]

ALL_TESTS = COMPULSORY_TESTS + PASSFAIL_TESTS

# What every generate_regfile test reads: the asap7 cells, their
# liberty for constant propagation, the base spec's module. Every test
# but generate_regfile_asap7 (whose own glob has it) adds the spec.
REGFILE_TEST_DATA = [
    "asap7/asap7_tech_1x_201209.lef",
    "asap7/asap7sc7p5t_28_R_1x_220121a.lef",
    "asap7/asap7sc7p5t_AO_RVT_FF_nldm_211120.lib.gz",
    "asap7/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz",
    "asap7/asap7sc7p5t_OA_RVT_FF_nldm_211120.lib.gz",
    "asap7/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib",
    "asap7/asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib.gz",
    "generate_regfile_asap7_rtl.v",
    "regfile_checks.tcl",
]

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
            "generate_regfile_asap7": REGFILE_TEST_DATA + [
                "generate_regfile_asap7_rtl_extra.v",
            ],
            "generate_regfile_clock_gate_asap7": REGFILE_TEST_DATA + [
                "generate_regfile_asap7.regfile",
            ],
            "generate_regfile_reset_asap7": REGFILE_TEST_DATA + [
                "generate_regfile_asap7.regfile",
            ],
            "generate_regfile_reset_high_asap7": REGFILE_TEST_DATA + [
                "generate_regfile_asap7.regfile",
            ],
            "generate_regfile_sv_ports_asap7": REGFILE_TEST_DATA + [
                "generate_regfile_asap7.regfile",
                "generate_regfile_sv_ports_asap7_rtl.sv",
            ],
            "generate_regfile_write_priority_asap7": REGFILE_TEST_DATA + [
                "generate_regfile_asap7.regfile",
            ],
            "make_7x7_2r1w_nangate45": [
                "make_7x7_2r1w_nangate45_behavioral.vok",
                "Nangate45/Nangate45.lef",
                "Nangate45/Nangate45_tech.lef",
                "Nangate45/Nangate45_typ.lib",
            ],
            "make_7x7_nangate45": [
                "make_7x7_nangate45_behavioral.vok",
                "Nangate45/Nangate45.lef",
                "Nangate45/Nangate45_tech.lef",
                "Nangate45/Nangate45_typ.lib",
            ],
            "make_8x8_2r1w_sky130": ["make_8x8_2r1w_sky130_behavioral.vok"],
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
