# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2025, The OpenROAD Authors
load("//test:regression.bzl", "regression_test")

package(features = ["layering_check"])

# From CMakeLists.txt or_integration_tests(TESTS
COMPULSORY_TESTS = [
    "aes_gf180",
    "allow_one_site_gaps",
    "avoid_overlap",
    "boundary_macros",
    "boundary_macros_auto_select",
    "boundary_macros_separate",
    "boundary_macros_separate2",
    "boundary_macros_tapcell",
    "cut_rows",
    "cut_rows_halo",
    "cut_rows_min_width",
    "cut_rows_with_endcaps",
    "disallow_one_site_gaps",
    "disallow_one_site_gaps2",
    "endcap_autoselection_error1",
    "endcap_autoselection_error2",
    "endcap_autoselection_error3",
    "endcap_autoselection_error4",
    "endcap_corners",
    "gcd_asap7",
    "gcd_fakeram",
    "gcd_nangate45",
    "gcd_prefix",
    "gcd_ripup",
    "gcd_sky130",
    "gcd_sky130_separate",
    "invalid_cells",
    "jpeg_gf180",
    "multiple_calls",
    "no_endcap",
    "overlap_cover",
    "region1",
    "symmetry",
]

# Disabled in CMakeLists.txt
MANUAL_TESTS = [
    "tap_man_tcl_check",
    "tap_readme_msgs_check",
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
        "Nangate45_data/Nangate45_stdcell.lef",
        "Nangate45_data/Nangate45_stdcell_x1.lef",
        "Nangate45_data/endcaps.lef",
        "Nangate45_data/endcaps_symmetric.lef",
        "Nangate45_data/macros.def",
        "asap7/asap7_tech_1x_201209.lef",
        "asap7/asap7sc7p5t_28_R_1x_220121a.lef",
        "asap7/asap7sc7p5t_AO_RVT_FF_nldm_211120.lib.gz",
        "asap7/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz",
        "asap7/asap7sc7p5t_OA_RVT_FF_nldm_211120.lib.gz",
        "asap7/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib",
        "asap7/asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib.gz",
        "asap7/setRC.tcl",
        "asap7_data/gcd.def",
        "boundary_macros_tapsplaced.def",
        "gcd_sky130hs_floorplan.def",
        "gf180/aes.def",
        "gf180/gf180mcu_5LM_1TM_9K_9t_sc.lef",
        "gf180/gf180mcu_5LM_1TM_9K_9t_tech.lef",
        "gf180/jpeg.def",
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
            "allow_one_site_gaps": ["disallow_one_site_gaps.def"],
            "boundary_macros": [],
            "disallow_one_site_gaps2": [
                "disallow_one_site_gaps.def",
                "disallow_one_site_gaps.defok",
            ],
            "gcd_sky130_separate": ["gcd_sky130.defok"],
            "multiple_calls": ["gcd_ripup.def"],
            "overlap_cover": ["sky130hs_data/cover.lef"],
        }.get(
            test_name,
            ["boundary_macros.def"],
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
