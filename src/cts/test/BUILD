load("@rules_cc//cc:cc_test.bzl", "cc_test")

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2025, The OpenROAD Authors
load("//test:regression.bzl", "regression_test")

package(features = ["layering_check"])

# From CMakeLists.txt or_integration_tests(TESTS
COMPULSORY_TESTS = [
    "array",
    "array_ins_delay",
    "array_max_wl",
    "array_no_blockages",
    "array_repair_clock_nets",
    "check_buffers",
    "check_buffers_blockages",
    "check_buffers_blockages_merge",
    "check_buffer_inference1",
    "check_buffer_inference2",
    "check_buffer_inference3",
    "check_charBuf",
    "check_max_fanout1",
    "check_max_fanout2",
    "check_max_fanout3",
    "check_wire_rc_cts",
    "clock_buffer_footprint",
    "dummy_load",
    "find_clock",
    #   cmake gives 17.47 um ave sink WL vs. bazel 11.54 um
    #    "find_clock_pad",
    "gated_clock1",
    "gated_clock2",
    "gated_clock3",
    "gated_clock4",
    "gated_clock5",
    "hier_insertion_delay",
    "insertion_delay",
    "inverters",
    "lvt_lib",
    "max_cap",
    "no_clocks",
    "no_sinks",
    "post_cts_opt",
    "simple_test",
    "simple_test_clustered",
    "simple_test_clustered_max_cap",
    "simple_test_hier",
    "sink_clustering_collision",
    "skip_nets",
    "twice",
]

# Disabled in CMakeLists.txt
MANUAL_TESTS = [
    "cts_man_tcl_check",
    "cts_readme_msgs_check",
]

ALL_TESTS = COMPULSORY_TESTS + MANUAL_TESTS

filegroup(
    name = "regression_resources",
    # Dependencies could be specified more narrowly per test case,
    # but at least it is not a glob of everything and there are
    # per test glob patterns below.
    srcs = [
        "Nangate45/Nangate45.lef",
        "Nangate45/Nangate45.rc",
        "Nangate45/Nangate45.tracks",
        "Nangate45/Nangate45.vars",
        "Nangate45/Nangate45_stdcell.lef",
        "Nangate45/Nangate45_tech.lef",
        "Nangate45/Nangate45_typ.lib",
        "helpers.tcl",
        "ihp-sg13g2/setRC.tcl",
        "ihp-sg13g2/sg13g2_stdcell.lef",
        "ihp-sg13g2/sg13g2_stdcell_typ_1p20V_25C.lib",
        "ihp-sg13g2/sg13g2_tech.lef",
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
            "array": [
                "array_tile.lef",
                "array_tile.lib",
            ],
            "array_dummy": [
                "array_tile.lef",
                "array_tile.lib",
            ],
            "array_full_flow": [
                "array_tile.lef",
                "array_tile.lib",
            ],
            "array_ins_delay": [
                "array_tile.lef",
                "array_tile_ins_delay.lib",
            ],
            "array_max_wl": [
                "array_tile.lef",
                "array_tile.lib",
            ],
            "array_no_blockages": [
                "array_tile.lef",
                "array_tile.lib",
            ],
            "array_repair_clock_nets": [
                "array_tile.lef",
                "array_tile.lib",
                "array_max_wl.defok",
            ],
            "check_buffer_inference1": [
                "ModNangate45/ModNangate45_typ.lib",
                "check_buffers.def",
            ],
            "check_buffer_inference2": [
                "ModNangate45/ModNangate45.lef",
                "check_buffers.def",
            ],
            "check_buffer_inference3": [
                "check_buffers.def",
            ],
            "check_charBuf": [
                "16sinks.def",
            ],
            "check_max_fanout1": [
                "ihp-sg13g2/setRC.tcl",
                "ihp-sg13g2/sg13g2_stdcell.lef",
                "ihp-sg13g2/sg13g2_stdcell_typ_1p20V_25C.lib",
                "ihp-sg13g2/sg13g2_tech.lef",
            ],
            "check_max_fanout2": [
                "check_buffers.def",
            ],
            "check_max_fanout3": [
                "ihp-sg13g2/setRC.tcl",
                "ihp-sg13g2/sg13g2_stdcell.lef",
                "ihp-sg13g2/sg13g2_stdcell_typ_1p20V_25C.lib",
                "ihp-sg13g2/sg13g2_tech.lef",
            ],
            "check_wire_rc_cts": [
                "check_buffers.def",
            ],
            "clock_buffer_footprint": [
                "ihp-sg13g2/setRC.tcl",
                "ihp-sg13g2/sg13g2_stdcell_typ_1p20V_25C.lib",
                "ihp-sg13g2/sg13g2_tech.lef",
                "ihp-sg13g2/sg13g2_stdcell.lef",
                "check_max_fanout3.def",
            ],
            "dummy_load": [
                "check_buffers.def",
            ],
            "find_clock": [
                "16sinks.def",
            ],
            "find_clock_pad": [
                "Nangate45/Nangate45.lef",
                "pad.lef",
                "Nangate45/Nangate45_typ.lib",
                "pad.lib",
            ],
            "gated_clock5": [
                "array_tile_ins_delay.lef",
                "array_tile_ins_delay.lib",
            ],
            "hier_insertion_delay": [
                "array_tile_ins_delay.lef",
                "array_tile_ins_delay.lib",
            ],
            "insertion_delay": [
                "array_tile_ins_delay.lef",
                "array_tile_ins_delay.lib",
            ],
            "inverters": [
                "array_tile_ins_delay.lef",
                "array_tile_ins_delay.lib",
            ],
            "lvt_lib": [
                "cts-helpers.tcl",
                "Nangate45/Nangate45_lvt.lef",
                "Nangate45/Nangate45_lvt.lib",
            ],
            "max_cap": [
                "//src/rsz/test:hi_fanout.tcl",
                "sky130hs/sky130hs.rc",
                "sky130hs/sky130hs.tlef",
                "sky130hs/sky130hs_std_cell.lef",
                "sky130hs/sky130hs_tt.lib",
            ],
            "no_clocks": [
                "no_clock.def",
            ],
            "post_cts_opt": [
                "cts-helpers.tcl",
            ],
            "simple_test": [
                "16sinks.def",
                "simple_test_out.defok",
            ],
            "simple_test_clustered": [
                "cts-helpers.tcl",
            ],
            "simple_test_clustered_max_cap": [
                "cts-helpers.tcl",
            ],
            "simple_test_hier": [
                "simple_test_hier_out.vok",
            ],
            "skip_nets": [
                "gated_clock2.def",
            ],
            "twice": [
                "16sinks.def",
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
    name = "cts_unittest",
    srcs = ["cts_unittest.cc"],
    features = ["-layering_check"],  # TODO: accesses private headers
    deps = [
        "//src/cts",
        "//src/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
