# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2025, The OpenROAD Authors

load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("//test:regression.bzl", "regression_test")

package(features = ["layering_check"])

# From CMakeLists.txt or_integration_tests(TESTS
COMPULSORY_TESTS = [
    "boundary_push1",
    "boundary_push2",
    "boundary_push3",
    "centralization1",
    "clocked_macro",
    "fixed_covers",
    "fixed_ios1",
    "fixed_macros1",
    "fixed_macros2",
    "guides1",
    "guides2",
    "io_constraints1",
    "io_constraints10",
    "io_constraints2",
    "io_constraints3",
    "io_constraints4",
    "io_constraints5",
    "io_constraints6",
    "io_constraints7",
    "io_constraints8",
    "io_constraints9",
    "io_pads1",
    "keep_clustering_data",
    "macro_only",
    "macros_without_pins1",
    "mixed_ios1",
    "no_unfixed_macros",
    "orientation_improve1",
    "orientation_improve2",
    "orientation_improve3",
    "unfixed_cells_dont_fit_in_core",
]

# Disabled in CMakeLists.txt
MANUAL_TESTS = [
    "mpl_man_tcl_check",
    "mpl_readme_msgs_check",
]

ALL_TESTS = COMPULSORY_TESTS + MANUAL_TESTS

filegroup(
    name = "regression_resources",
    # Dependencies could be specified more narrowly per test case,
    # but at least it is not a glob of everything and there are
    # per test glob patterns below.
    srcs = [
        "Nangate45/Nangate45.lef",
        "Nangate45/Nangate45_tech.lef",
        "helpers.tcl",
    ],
)

[
    filegroup(
        name = test_name + "_resources",
        srcs = [":regression_resources"] + glob(
            [
                test_name + ".*",
            ] + {
                "boundary_push1": [
                    "testcases/boundary_push1.def",
                    "testcases/orientation_improve1.lef",
                ],
                "boundary_push2": [
                    "testcases/boundary_push2.def",
                    "testcases/orientation_improve1.lef",
                ],
                "boundary_push3": [
                    "testcases/boundary_push2.def",
                    "testcases/orientation_improve1.lef",
                ],
                "centralization1": [
                    "testcases/centralization1.def",
                    "testcases/orientation_improve1.lef",
                ],
                "clocked_macro": [
                    "testcases/clocked_macro.def",
                    "testcases/clocked_macro.lef",
                    "testcases/clocked_macro.lib",
                    "testcases/clocked_macro.v",
                ],
                "fixed_covers": [
                    "Nangate45_io/dummy_pads.lef",
                    "testcases/fixed_macros1.def",
                    "testcases/orientation_improve1.lef",
                ],
                "fixed_ios1": [
                    "Nangate45/Nangate45_fast.lib",
                    "testcases/fixed_ios1.def",
                    "testcases/macro_only.lef",
                    "testcases/macro_only.lib",
                ],
                "fixed_macros1": [
                    "testcases/fixed_macros1.def",
                    "testcases/orientation_improve1.lef",
                ],
                "fixed_macros2": [
                    "testcases/fixed_macros2.def",
                    "testcases/orientation_improve1.lef",
                ],
                "guides1": [
                    "testcases/guides1.def",
                    "testcases/macro_only.lef",
                    "testcases/macro_only.lib",
                ],
                "guides2": [
                    "testcases/macro_only.def",
                    "testcases/macro_only.lef",
                    "testcases/macro_only.lib",
                    "testcases/macro_only.v",
                ],
                "io_constraints1": [
                    "testcases/io_constraints1.def",
                    "testcases/macro_only.lef",
                    "testcases/macro_only.lib",
                ],
                "io_constraints10": [
                    "testcases/io_constraints1.def",
                    "testcases/macro_only.lef",
                    "testcases/macro_only.lib",
                ],
                "io_constraints2": [
                    "testcases/io_constraints1.def",
                    "testcases/macro_only.lef",
                    "testcases/macro_only.lib",
                ],
                "io_constraints3": [
                    "testcases/io_constraints3.def",
                    "testcases/macro_only.lef",
                    "testcases/macro_only.lib",
                ],
                "io_constraints4": [
                    "testcases/io_constraints4.def",
                    "testcases/macro_only.lef",
                    "testcases/macro_only.lib",
                ],
                "io_constraints5": [
                    "testcases/io_constraints4.def",
                    "testcases/macro_only.lef",
                    "testcases/macro_only.lib",
                ],
                "io_constraints6": [
                    "Nangate45/Nangate45_fast.lib",
                    "testcases/io_constraints6.def",
                    "testcases/macro_only.lef",
                ],
                "io_constraints7": [
                    "Nangate45/Nangate45_fast.lib",
                    "testcases/io_constraints6.def",
                    "testcases/macro_only.lef",
                ],
                "io_constraints8": [
                    "Nangate45/Nangate45_fast.lib",
                    "testcases/io_constraints6.def",
                    "testcases/macro_only.lef",
                ],
                "io_constraints9": [
                    "testcases/io_constraints1.def",
                    "testcases/macro_only.lef",
                    "testcases/macro_only.lib",
                ],
                "io_pads1": [
                    "Nangate45_io/dummy_pads.lef",
                    "testcases/io_pads1.def",
                    "testcases/macro_only.lef",
                ],
                "keep_clustering_data": [
                    "Nangate45/Nangate45_fast.lib",
                    "testcases/io_constraints6.def",
                    "testcases/macro_only.lef",
                ],
                "macro_only": [
                    "testcases/macro_only.def",
                    "testcases/macro_only.lef",
                    "testcases/macro_only.lib",
                    "testcases/macro_only.v",
                ],
                "macros_without_pins1": [
                    "Nangate45_io/dummy_pads.lef",
                    "testcases/macros_without_pins1.def",
                    "testcases/orientation_improve1.lef",
                ],
                "mixed_ios1": [
                    "Nangate45/Nangate45_fast.lib",
                    "testcases/macro_only.lef",
                    "testcases/macro_only.lib",
                    "testcases/mixed_ios1.def",
                ],
                "no_unfixed_macros": [
                    "testcases/macro_only.lef",
                    "testcases/macro_only.lib",
                    "testcases/macro_only.v",
                    "testcases/no_unfixed_macros.def",
                ],
                "orientation_improve1": [
                    "testcases/orientation_improve1.def",
                    "testcases/orientation_improve1.lef",
                ],
                "orientation_improve2": [
                    "testcases/orientation_improve1.def",
                    "testcases/orientation_improve1.lef",
                ],
                "orientation_improve3": [
                    "testcases/orientation_improve1.def",
                    "testcases/orientation_improve1.lef",
                ],
                "unfixed_cells_dont_fit_in_core": [
                    "testcases/macro_only.lef",
                    "testcases/unfixed_cells_dont_fit_in_core.def",
                ],
            }.get(test_name, []),
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

cc_test(
    name = "mpl_snapper_unittest",
    srcs = [
        "cpp/MplTest.h",
        "cpp/TestSnapper.cpp",
    ],
    features = ["-layering_check"],  # TODO: includes private headers
    deps = [
        "//src/mpl",
        "//src/odb",
        "//src/tst",
        "//src/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
