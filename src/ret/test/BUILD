# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("//test:regression.bzl", "regression_test")

package(features = ["layering_check"])

TESTS = [
    "retime_basic",
    "retime_dsec_deep",
    "retime_dsec_enable",
    "retime_dsec_enable_chain",
    "retime_dsec_existing_names",
    "retime_dsec_inverting",
    "retime_dsec_inverting_asap7",
    "retime_dsec_loop",
    "retime_dsec_plain",
    "retime_gated_clock",
    "retime_input_limit",
    "retime_input_path",
    "retime_latch",
    "retime_negedge",
    "retime_no_base_cell",
    "retime_no_clock",
    "retime_no_moves",
    "retime_two_clocks",
    "retime_unmap",
    "unmap_basic",
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
    data = [":test_resources"],
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
        "//src/dbSta",
        "//src/dbSta:dbNetwork",
        "//src/odb/src/db",
        "//src/ret",
        "//src/sta:opensta_lib",
        "//src/tst",
        "//src/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
