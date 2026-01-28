# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2025, The OpenROAD Authors

load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("//test:regression.bzl", "regression_test")

package(features = ["layering_check"])

# From CMakeLists.txt or_integration_tests(TESTS
COMPULSORY_TESTS = [
    "down_access_term",
    "drc_test",
    "ispd18_sample",
    "ispd18_sample_incr",
    "ndr_vias1",
    "ndr_vias2",
    "ndr_vias3",
    "obstruction",
    "single_step",
    "ta_ap_aligned",
    "ta_pin_aligned",
    "top_level_term",
    "top_level_term2",
    "via_access_layer",
]

# Disabled in CMakeLists.txt
MANUAL_TESTS = [
    "drt_man_tcl_check",
    "drt_readme_msgs_check",
]

# TODO: Enable once difference between bazel and ctest is resolved.
MANUAL_FOR_BAZEL_TESTS = [
    "down_access_term",
    "drc_test",
    "ispd18_sample",
    "ispd18_sample_incr",
    "ndr_vias1",
    "ndr_vias2",
    "ndr_vias3",
    "single_step",
    "top_level_term2",
]

ALL_TESTS = COMPULSORY_TESTS + MANUAL_TESTS

filegroup(
    name = "regression_resources",
    # Dependencies could be specified more narrowly per test case,
    # but at least it is not a glob of everything and there are
    # per test glob patterns below.
    srcs = [
        "Nangate45/Nangate45_stdcell.lef",
        "Nangate45/Nangate45_tech.lef",
        "gcd_nangate45.route_guide",
        "gcd_nangate45.tcl",
        "gcd_nangate45_distributed.tcl",
        "gcd_nangate45_dump_worker.tcl",
        "gcd_nangate45_preroute.def",
        "gcd_nangate45_test_worker.tcl",
        "gcd_sky130hd.def",
        "gcd_sky130hd.guide",
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
        "testcase/ispd18_sample/ispd18_sample.input.def",
        "testcase/ispd18_sample/ispd18_sample.input.guide",
        "testcase/ispd18_sample/ispd18_sample.input.lef",
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
            "ispd18_sample.defok",
        ] if test_name in [
            "ispd18_sample_incr",
            "single_step",
        ] else []),
    )
    for test_name in ALL_TESTS
]

[
    regression_test(
        name = test_name,
        data = [":" + test_name + "_resources"],
        tags = ["manual"] if test_name in MANUAL_TESTS or
                             test_name in MANUAL_FOR_BAZEL_TESTS else [],
        visibility = ["//visibility:public"],
    )
    for test_name in ALL_TESTS
]

cc_test(
    name = "gc_unittest",
    srcs = [
        "fixture.cpp",
        "fixture.h",
        "gcTest.cpp",
    ],
    features = ["-layering_check"],  # TODO: acceses private headers
    includes = [
        "../src",
    ],
    deps = [
        "//src/drt",
        "//src/drt:db_hdrs",
        "//src/odb",
        "//src/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
