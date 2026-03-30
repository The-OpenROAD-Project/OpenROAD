# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2025, The OpenROAD Authors

# Test Parity Note: All tests from CMakeLists.txt are included in this BUILD file.
# No tests are intentionally excluded. See .kiro/specs/bazel-cmake-test-parity/intentional_exclusions.md

load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("@rules_python//python:defs.bzl", "py_library", "py_test")
load("//test:regression.bzl", "doc_check_test", "regression_rule_test", "regression_test")

package(features = ["layering_check"])

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

ALL_TESTS = COMPULSORY_TESTS

PY_TESTS = [
    "ispd18_sample",
    "single_step",
    "top_level_term",
    "top_level_term2",
]

filegroup(
    name = "regression_resources",
    # Dependencies could be specified more narrowly per test case,
    # but at least it is not a glob of everything and there are
    # per test glob patterns below.
    srcs = [
        "Nangate45/Nangate45_stdcell.lef",
        "Nangate45/Nangate45_tech.lef",
        "drt_aux.py",
        "gcd_nangate45.route_guide",
        "gcd_nangate45.tcl",
        "gcd_nangate45_distributed.tcl",
        "gcd_nangate45_dump_worker.tcl",
        "gcd_nangate45_preroute.def",
        "gcd_nangate45_test_worker.tcl",
        "gcd_sky130hd.def",
        "gcd_sky130hd.guide",
        "helpers.py",
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
        visibility = ["//visibility:public"],
    )
    for test_name in ALL_TESTS
]

py_library(
    name = "helpers",
    srcs = [
        "drt_aux.py",
        "helpers.py",
    ],
    imports = ["."],
    deps = [
        "//python/openroad:openroadpy",
    ],
)

[
    py_test(
        name = test_name,
        srcs = [test_name + ".py"],
        data = [":" + test_name + "_resources"],
        deps = [
            ":helpers",
            "//python/openroad:openroadpy",
        ],
    )
    for test_name in PY_TESTS
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

py_test(
    name = "drt_man_tcl_check",
    srcs = ["drt_man_tcl_check.py"],
    data = ["//src/drt:doc_files"],
    tags = ["doc_check"],
    deps = ["//docs/src/scripts:extract_utils"],
)

doc_check_test(
    name = "drt_readme_msgs_check",
    data = [
        "//src/drt:doc_files",
        "//src/drt:messages_txt",
    ],
    visibility = ["//visibility:public"],
)

regression_rule_test(
    name = "aes_nangate45-py_test",
    bazel_test_sh = "//test:bazel_test.sh",
    check_log = False,
    data = [
        "aes_nangate45.route_guide",
        "aes_nangate45_preroute.def",
        ":regression_resources",
    ],
    openroad = "//:openroad",
    regression_test = "//test:regression_test.sh",
    tags = ["manual"],
    test_file = "aes_nangate45.py",
    test_name = "aes_nangate45",
    visibility = ["//visibility:public"],
)
