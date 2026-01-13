# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2025, The OpenROAD Authors

load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("//test:regression.bzl", "regression_test")

package(features = ["layering_check"])

# From CMakeLists.txt or_integration_tests(TESTS
COMPULSORY_TESTS = [
    "aes",
    "blockage01",
    "cell_on_block1",
    "cell_on_block2",
    "check1",
    "check2",
    "check3",
    "check4",
    "check5",
    "check6",
    "check7",
    "check8",
    "check9",
    "fence01",
    "fence02",
    "fence03",
    "fillers1",
    "fillers2",
    "fillers2_verbose",
    "fillers3",
    "fillers4",
    "fillers5",
    "fillers6",
    "fillers7",
    "fillers8",
    "fillers9",
    "fillers9_verbose",
    "fillers10",
    "fragmented_row01",
    "fragmented_row02",
    "fragmented_row03",
    "fragmented_row04",
    "gcd",
    "hybrid_cells",
    "hybrid_cells2",
    "ibex",
    "max_disp1",
    "mirror1",
    "mirror2",
    "mirror3",
    "multi_height_one_site_gap_disallow",
    "multi_height_rows",
    "obstruction1",
    "obstruction2",
    "one_site_gap_disallow",
    "pad01",
    "pad02",
    "pad03",
    "pad04",
    "pad05",
    "pad06",
    "pad07",
    "pad08",
    "pad09",
    "regions1",
    "regions2",
    "regions3",
    "report_failures",
    "simple01",
    "simple02",
    "simple03",
    "simple04",
    "simple05",
    "simple07",
    "simple08",
    "simple09",
    "simple10",
    "edge_spacing",
    "aes-opt",
    "blockage1-opt",
    "gcd-opt",
    "gcd_no_one_site_gaps-opt",
    "ibex-opt",
    "multi_height1-opt",
    "edge_spacing-opt",
    "regions1-opt",
    "regions2-opt",
]

# Disabled in CMakeLists.txt
MANUAL_TESTS = [
    "dpl_man_tcl_check",
    "dpl_readme_msgs_check",
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
        "Nangate45_data/Nangate45.lef",
        "Nangate45_data/Nangate45-opt.lef",
        "Nangate45_data/fake_macros.lef",
        "aes_cipher_top_replace.def",
        "block.lef",
        "block2.lef",
        "edge_spacing.lef",
        "extra.lef",
        "fill3.lef",
        "gcd_nangate45.tcl",
        "gcd_replace.def",
        "helpers.tcl",
        "ibex_core_replace.def",
        "low_util01.tcl",
        "low_util02.tcl",
        "low_util03.tcl",
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
        srcs = [":regression_resources"] + glob(
            [
                test_name + ".*",
            ],
        ) + {
            "check1": [
                "simple01.def",
            ],
            "check7": [
                "check6.def",
            ],
            "edge_spacing": [
                "multi_height_rows.def",
            ],
            "fillers1": [
                "simple01.def",
            ],
            "fillers10": [
                "simple09.def",
            ],
            "fillers2": [
                "simple01.def",
            ],
            "fillers2_verbose": [
                "simple01.def",
            ],
            "fillers3": [
                "simple01.def",
            ],
            "fillers5": [
                "fragmented_row04.def",
            ],
            "fillers7": [
                "simple01.def",
            ],
            "fillers9_verbose": [
                "fillers9.lef",
                "fillers9.def",
            ],
            "pad01": [
                "simple01.def",
            ],
            "pad02": [
                "simple01.def",
            ],
            "pad03": [
                "simple03.def",
            ],
            "pad05": [
                "extra.lef",
                "check6.def",
            ],
            "pad06": [
                "check6.def",
            ],
            "pad07": [
                "simple01.def",
            ],
            "pad08": [
                "simple01.def",
            ],
        }.get(test_name, []),
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
    name = "dpl_unittest",
    srcs = ["dpl_test.cc"],
    deps = [
        "//src/dpl",
        "//src/odb",
        "//src/tst",
        "//src/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
