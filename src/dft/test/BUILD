# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2025, The OpenROAD Authors

load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("//test:regression.bzl", "regression_test")

package(features = ["layering_check"])

# From CMakeLists.txt or_integration_tests(TESTS
COMPULSORY_TESTS = [
    "max_chain_count_sky130",
    "one_cell_sky130",
    "place_sort_sky130",
    "scan_architect_clock_mix_sky130",
    "scan_architect_no_mix_sky130",
    "scan_architect_register_bank_no_clock_mix_sky130",
    "scandef_core_sky130",
    "scandef_sky130",
    "sub_modules_sky130",
]

# Disabled in CMakeLists.txt
MANUAL_TESTS = [
    "dft_man_tcl_check",
    "dft_readme_msgs_check",
]

ALL_TESTS = COMPULSORY_TESTS + MANUAL_TESTS

filegroup(
    name = "regression_resources",
    srcs = [
        "helpers.tcl",
        "sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib",
        "sky130hd/sky130_fd_sc_hd_merged.lef",
        "sky130hd/sky130hd.tlef",
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
            "scan_architect_clock_mix_sky130": [
                "scan_architect_sky130.v",
            ],
            "scan_architect_no_mix_sky130": [
                "scan_architect_sky130.v",
            ],
            "scan_architect_register_bank_no_clock_mix_sky130": [
                "register_bank_top.v",
            ],
            "scandef_core_sky130": [
                "register_bank_cores.lef",
                "scan_inserted_register_bank.v",
            ],
            "scandef_sky130": [
                "scan_inserted_design_sky130.scandef",
                "scan_inserted_design_sky130.v",
            ],
        }.get(test_name, []),
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

cc_test(
    name = "dft_scan_architect_unittest",
    srcs = ["cpp/TestScanArchitect.cpp"],
    features = ["-layering_check"],  # TODO: includes private headers
    deps = [
        "//src/dft",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "dft_scan_architect_heuristic_unittest",
    srcs = [
        "cpp/ScanCellMock.cpp",
        "cpp/ScanCellMock.hh",
        "cpp/TestScanArchitectHeuristic.cpp",
    ],
    features = ["-layering_check"],  # TODO: includes private headers
    deps = [
        "//src/dft",
        "//src/dft/src/utils",
        "//src/odb",
        "//src/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
