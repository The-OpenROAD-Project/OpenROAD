# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors

load("@rules_python//python:defs.bzl", "py_test")
load("//test:regression.bzl", "regression_test")

package(features = ["layering_check"])

# --- Downstream API surface tests ---
# Verify visibility labels and dev_dependency markers by inspecting
# BUILD.bazel and MODULE.bazel directly. Instant, no downstream workspace.

py_test(
    name = "openroad_public_test",
    srcs = ["check_visibility.py"],
    args = [
        "--build-file=$(location //:BUILD.bazel)",
        "--rule-type=cc_binary",
        "--target=openroad",
        "--expect=public",
    ],
    data = ["//:BUILD.bazel"],
    main = "check_visibility.py",
)

py_test(
    name = "openroad_py_public_test",
    srcs = ["check_visibility.py"],
    args = [
        "--build-file=$(location //:BUILD.bazel)",
        "--rule-type=py_library",
        "--target=openroad_py",
        "--expect=public",
    ],
    data = ["//:BUILD.bazel"],
    main = "check_visibility.py",
)

py_test(
    name = "openroad_lib_restricted_test",
    srcs = ["check_visibility.py"],
    args = [
        "--build-file=$(location //:BUILD.bazel)",
        "--rule-type=cc_library",
        "--target=openroad_lib",
        "--expect=not_public",
    ],
    data = ["//:BUILD.bazel"],
    main = "check_visibility.py",
)

py_test(
    name = "rules_verilator_dev_dep_test",
    srcs = ["check_dev_dep.py"],
    args = [
        "--module-file=$(location //:MODULE.bazel)",
        "--dep=rules_verilator",
    ],
    data = ["//:MODULE.bazel"],
    main = "check_dev_dep.py",
)

py_test(
    name = "verilator_dev_dep_test",
    srcs = ["check_dev_dep.py"],
    args = [
        "--module-file=$(location //:MODULE.bazel)",
        "--dep=verilator",
    ],
    data = ["//:MODULE.bazel"],
    main = "check_dev_dep.py",
)

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

py_test(
    name = "timing_report_api_test",
    size = "medium",
    srcs = ["timing_report_api.py"],
    data = [":regression_resources"],
    main = "timing_report_api.py",
    deps = ["//python/openroad:openroadpy"],
)

# These tests take a long time, make them easy to explicitly invoke
test_suite(
    name = "flow_tests",
    tags = ["manual"],
    tests = [test_name + "-tcl" for test_name in BIG_TESTS],
)
