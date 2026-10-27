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
    "write_ports_nangate45",
    "and_or_read_asap7",
    "zero_word_asap7",
    "reset_asap7",
    "reset_nangate45",
    "banks_asap7",
    "banked_read_ports_asap7",
    "register_read_address_asap7",
    "write_mux_asap7",
    "periphery_unplaced_asap7",
    "cells_name_asap7",
    "decoder_word_asap7",
    "bit_folds_asap7",
    "periphery_verilog_asap7",
    "write_priority_or_asap7",
    "service_columns_asap7",
    "bank_column_layout_asap7",
    "option_errors_asap7",
]

ALL_TESTS = COMPULSORY_TESTS + PASSFAIL_TESTS

# Local tests, too slow to run in CI (minutes each); run them by name.
MANUAL_TESTS = [
    "banks_asap7",
    "bank_column_layout_asap7",
    "write_priority_or_asap7",
]

filegroup(
    name = "regression_resources",
    # Dependencies could be specified more narrowly per test case,
    # but at least it is not a glob of everything and there are
    # per test glob patterns below.
    srcs = [
        "helpers.tcl",
        "ram_sim.tcl",
        "routed.tcl",
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
            "and_or_read_asap7": [
                "//test:asap7_data",
                "//test:asap7_nldm_data",
            ],
            "bank_column_layout_asap7": [
                "//test:asap7_data",
                "//test:asap7_nldm_data",
            ],
            "banked_read_ports_asap7": [
                "//test:asap7_data",
                "//test:asap7_nldm_data",
            ],
            "banks_asap7": [
                "//test:asap7_data",
                "//test:asap7_nldm_data",
            ],
            "bit_folds_asap7": [
                "//test:asap7_data",
                "//test:asap7_nldm_data",
            ],
            "cells_name_asap7": [
                "//test:asap7_data",
                "//test:asap7_nldm_data",
            ],
            "decoder_word_asap7": [
                "//test:asap7_data",
                "//test:asap7_nldm_data",
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
            "option_errors_asap7": [
                "//test:asap7_data",
                "//test:asap7_nldm_data",
            ],
            "periphery_unplaced_asap7": [
                "//test:asap7_data",
                "//test:asap7_nldm_data",
            ],
            "periphery_verilog_asap7": [
                "//test:asap7_data",
                "//test:asap7_nldm_data",
            ],
            "register_read_address_asap7": [
                "//test:asap7_data",
                "//test:asap7_nldm_data",
            ],
            "reset_asap7": [
                "//test:asap7_data",
                "//test:asap7_nldm_data",
            ],
            "reset_nangate45": [
                "Nangate45/Nangate45.lef",
                "Nangate45/Nangate45_tech.lef",
                "Nangate45/Nangate45_typ.lib",
            ],
            "service_columns_asap7": [
                "//test:asap7_data",
                "//test:asap7_nldm_data",
            ],
            "write_mux_asap7": [
                "//test:asap7_data",
                "//test:asap7_nldm_data",
            ],
            "write_ports_nangate45": [
                "Nangate45/Nangate45.lef",
                "Nangate45/Nangate45_tech.lef",
                "Nangate45/Nangate45_typ.lib",
            ],
            "write_priority_or_asap7": [
                "//test:asap7_data",
                "//test:asap7_nldm_data",
            ],
            "zero_word_asap7": [
                "//test:asap7_data",
                "//test:asap7_nldm_data",
            ],
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
        tags = ["manual"] if test_name in MANUAL_TESTS else [],
        visibility = ["//visibility:public"],
    )
    for test_name in ALL_TESTS
]
