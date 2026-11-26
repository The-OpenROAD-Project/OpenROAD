# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("//bazel:man_tcl_checker.bzl", "man_tcl_check_test")
load("//test:regression.bzl", "doc_check_test", "regression_test")

package(features = ["layering_check"])

TESTS = [
    "retime_basic",
    "retime_clock_errors",
    "retime_dont_touch",
    "retime_dsec_async_tied",
    "retime_dsec_class_groups",
    "retime_dsec_deep",
    "retime_dsec_dont_touch_group",
    "retime_dsec_dont_use",
    "retime_dsec_dont_use_tie",
    "retime_dsec_enable",
    "retime_dsec_enable_chain",
    "retime_dsec_enable_driver",
    "retime_dsec_enable_gate",
    "retime_dsec_enable_only",
    "retime_dsec_existing_names",
    "retime_dsec_hier",
    "retime_dsec_hier_tied_base",
    "retime_dsec_hier_unmap",
    "retime_dsec_hier_unmap_nested",
    "retime_dsec_hier_unmap_top",
    "retime_dsec_inverting",
    "retime_dsec_inverting_asap7",
    "retime_dsec_loop",
    "retime_dsec_port_fed",
    "retime_dsec_port_only",
    "retime_dsec_port_tap",
    "retime_dsec_tie_stays",
    "retime_dsec_tied_base",
    "retime_dsec_unmapped_neighbor",
    "retime_gated_clock",
    "retime_gated_clock_deep",
    "retime_gated_clock_enable",
    "retime_gated_clock_expr",
    "retime_gated_clock_order",
    "retime_hierarchical",
    "retime_hold",
    "retime_input_limit",
    "retime_input_path",
    "retime_latch",
    "retime_max_moves",
    "retime_negedge",
    "retime_no_base_cell",
    "retime_no_moves",
    "retime_no_register",
    "retime_open_input",
    "retime_place_size",
    "retime_reconverge",
    "retime_routed",
    "retime_scan_chain",
    "retime_scan_set",
    "retime_shared_net",
    "retime_slack_search",
    "retime_sta_unmap_undo",
    "retime_tristate",
    "retime_unmap",
    "retime_wire",
]

filegroup(
    name = "test_resources",
    srcs = glob(
        ["**/*"],
        exclude = [
            test + "." + ext
            for test in TESTS
            for ext in [
                "tcl",
                "py",
            ]
        ],
    ),
)

[regression_test(
    name = test_name,
    data = [
        ":test_resources",
        "@abc//:abc_bin",
    ],
) for test_name in TESTS]

cc_test(
    name = "TestSequentialUnmapper",
    srcs = [
        "SequentialUnmapperFixture.h",
        "TestSequentialUnmapperCells.cpp",
        "TestSequentialUnmapperRun.cpp",
    ],
    data = glob([
        "sequnmap_*.lef",
        "sequnmap_*.lib",
    ]),
    deps = [
        "//openroad/dbSta",
        "//openroad/dbSta:dbNetwork",
        "//openroad/odb/src/db",
        "//openroad/ret:ret_internal",
        "//openroad/sta:opensta_lib",
        "//openroad/tst",
        "//openroad/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

man_tcl_check_test(
    name = "ret_man_tcl_check",
    module = "ret",
)

doc_check_test(
    name = "ret_readme_msgs_check",
    data = [
        "//openroad/ret:doc_files",
        "//openroad/ret:messages_txt",
    ],
    visibility = ["//visibility:public"],
)
