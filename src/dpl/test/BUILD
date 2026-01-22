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
    srcs = [
        "Nangate45/Nangate45.lef",
        "helpers.tcl",
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
            "aes": [
                "aes_cipher_top_replace.def",
            ],
            "blockage1-opt": [
                "sky130hd/sky130hd.tlef",
                "sky130hd/sky130hd_std_cell.lef",
            ],
            "cell_on_block1": [
                "block2.lef",
            ],
            "cell_on_block2": [
                "block2.lef",
                "sky130hd/sky130hd.tlef",
                "sky130hd/sky130hd_std_cell.lef",
            ],
            "check1": [
                "simple01.def",
            ],
            "check5": [
                "extra.lef",
            ],
            "check6": [
                "extra.lef",
            ],
            "check7": [
                "check6.def",
                "extra.lef",
            ],
            "check8": [
                "extra.lef",
            ],
            "check9": [
                "extra.lef",
            ],
            "edge_spacing": [
                "multi_height_rows.def",
                "Nangate45/fake_macros.lef",
            ],
            "edge_spacing-opt": [
                "edge_spacing.lef",
                "multi_height_rows.def",
                "Nangate45/fake_macros.lef",
            ],
            "fillers1": [
                "simple01.def",
            ],
            "fillers10": [
                "simple09.def",
                "Nangate45/fakeram45_64x7.lef",
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
            "fillers4": [
                "fill3.lef",
            ],
            "fillers5": [
                "fragmented_row04.def",
            ],
            "fillers6": [
                "fill3.lef",
            ],
            "fillers7": [
                "simple01.def",
            ],
            "fillers9_verbose": [
                "fillers9.lef",
                "fillers9.def",
            ],
            "gcd": [
                "gcd_replace.def",
            ],
            "gcd_nangate45": [
                "gcd_replace.def",
            ],
            "gcd_no_one_site_gaps-opt": [
                "Nangate45_data/Nangate45-opt.lef",
            ],
            "hybrid_cells": [
                "Nangate45/fake_macros.lef",
            ],
            "hybrid_cells2": [
                "Nangate45/fake_macros.lef",
            ],
            "ibex": [
                "ibex_core_replace.def",
            ],
            "low_util01": [
                "gcd_replace.def",
            ],
            "low_util02": [
                "aes_cipher_top_replace.def",
            ],
            "low_util03": [
                "ibex_core_replace.def",
            ],
            "max_disp1": [
                "Nangate45/fakeram45_1024x32.lef",
            ],
            "mirror1": [
                "gcd_replace.def",
            ],
            "mirror2": [
                "extra.lef",
            ],
            "mirror3": [
                "gcd_replace.def",
            ],
            "multi_height1-opt": [
                "Nangate45/fake_macros.lef",
            ],
            "multi_height_one_site_gap_disallow": [
                "Nangate45/fake_macros.lef",
                "Nangate45_data/Nangate45.lef",
                "Nangate45_data/fake_macros.lef",
            ],
            "multi_height_rows": [
                "Nangate45/fake_macros.lef",
            ],
            "obstruction2": [
                "Nangate45/fakeram45_64x7.lef",
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
                "extra.lef",
                "check6.def",
            ],
            "pad07": [
                "simple01.def",
            ],
            "pad08": [
                "simple01.def",
            ],
            "pad09": [
                "sky130hs/sky130hs.tlef",
                "sky130hs/sky130hs_std_cell.lef",
            ],
            "regions3": [
                "Nangate45/fake_macros.lef",
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
