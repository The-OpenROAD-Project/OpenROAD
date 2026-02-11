# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors

load("//test:regression.bzl", "regression_test")

package(features = ["layering_check"])

exports_files(
    [
        "bazel_test.sh",
        "regression_test.sh",
    ],
    visibility = ["//visibility:public"],
)

filegroup(
    name = "nangate45_data",
    srcs = [
        "Nangate45/Nangate45.lef",
        "Nangate45/Nangate45_typ.lib",
        "Nangate45/fakeram45_64x7.lef",
        "Nangate45/fakeram45_64x7.lib",
    ],
    visibility = ["//visibility:public"],
)

filegroup(
    name = "sky130hd_data",
    srcs = [
        "sky130hd/sky130_fd_sc_hd_merged.lef",
        "sky130hd/sky130hd.tlef",
        "sky130hd/sky130hd_std_cell.lef",
    ],
    visibility = ["//visibility:public"],
)

filegroup(
    name = "ihp-sg13g2_data",
    srcs = [
        "ihp-sg13g2/IOLib.lef",
        "ihp-sg13g2/IOLib_dummy.lib",
        "ihp-sg13g2/setRC.tcl",
        "ihp-sg13g2/sg13g2_stdcell.lef",
        "ihp-sg13g2/sg13g2_stdcell_typ_1p20V_25C.lib",
        "ihp-sg13g2/sg13g2_stdcell_typ_1p50V_25C.lib",
        "ihp-sg13g2/sg13g2_tech.lef",
    ],
    visibility = ["//visibility:public"],
)

filegroup(
    name = "regression_resources",
    # TODO Refine glob later.
    #
    # This is a very broad glob, but most of the data in size
    # comes from .lib files and we can't know which pdk files
    # to include for a test given the name only.
    srcs = glob(["**/*"]),
    visibility = ["//visibility:public"],
)

# From CMakeLists.txt or_integration_tests(TESTS
PASSFAIL_TESTS = [
    "commands_without_load",
]

COMPULSORY_TESTS = [
    "error1",
    "get_core_die_areas",
    "ord_tclsh_completion",
    "place_inst",
    "upf_test",
    "upf_aes",
    "write_db",
] + PASSFAIL_TESTS

# TODO: Enable once difference between bazel and ctest is resolved.
MANUAL_FOR_BAZEL_TESTS = [
    "upf_test",
    "upf_aes",
]

# Bazel OpenROAD is not compiled with Python support yet
# leave as manual for now.
PYTHON_TESTS = [
    "timing_api",
    "timing_api_2",
    "timing_api_3",
    "timing_api_4",
    "two_designs",
]

# The local Bazel OpenROAD workflow for a quick smoketest
# is to run, in the relevant folder:
#
# "bazel test -c opt ..."
#
# Now, or_integration_tests() in CMakeLists.txt do not list
# BIG_TESTS below.
#
# By marking the tests tags=["manual"],
# the tests are available, but have to be invoked
# explicitly giving CI control over when they are executed,
# which can be useful because the tests are long running.
#
# TODO: Create golden ok files for these tests.
BIG_TESTS = [
    "aes_sky130hd",
    "aes_sky130hs",
    "gcd_sky130hd_fast_slow",
    "gcd_sky130hd",
    "gcd_sky130hs",
    "gcd_asap7",
    "aes_asap7",
    "ibex_sky130hd",
    "ibex_sky130hs",
    "jpeg_sky130hd",
    "jpeg_sky130hs",
    "aes_nangate45",
    "gcd_nangate45",
    "tinyRocket_nangate45",
]

[
    regression_test(
        name = test_name,
        size = "enormous" if test_name in BIG_TESTS else "medium",
        check_log = False if test_name in PASSFAIL_TESTS else True,
        check_passfail = True if test_name in PASSFAIL_TESTS else False,
        tags = [] if test_name in COMPULSORY_TESTS and test_name not in MANUAL_FOR_BAZEL_TESTS else ["manual"],
    )
    for test_name in COMPULSORY_TESTS + PYTHON_TESTS + BIG_TESTS
]

# These tests take a long time, make them easy to explicitly invoke
test_suite(
    name = "flow_tests",
    tags = ["manual"],
    tests = [test_name + "-tcl" for test_name in BIG_TESTS],
)
