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

# From CMakeLists.txt or_integration_tests(PASSFAIL_TESTS
PASSFAIL_TESTS = [
    "gcd_nangate45_distributed_multiworker",
]

# Single-host distributed detailed-route test. It spawns a leader + 2 worker
# processes + a balancer (binding localhost sockets on dynamically-allocated
# ports) and asserts the distributed routed DEF equals the non-distributed
# baseline. Registered and verified under CMake/ctest; tagged "manual" +
# "requires-network" for Bazel because the sandbox may forbid opening sockets.
DISTRIBUTED_TESTS = [
    "gcd_nangate45_distributed_local",
]

ALL_TESTS = COMPULSORY_TESTS + PASSFAIL_TESTS

BIG_TESTS = [
    "ndr_vias1",
    "ndr_vias2",
    "gcd_nangate45_distributed_multiworker",
]

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
        "distributed_balancer.tcl",
        "distributed_multiworker_harness.tcl",
        "distributed_worker.tcl",
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
        size = "large" if test_name in BIG_TESTS else "small",
        check_passfail = True if test_name in PASSFAIL_TESTS else False,
        data = [":" + test_name + "_resources"],
        visibility = ["//visibility:public"],
    )
    for test_name in ALL_TESTS
]

# Single-host distributed detailed-route regression. The harness re-execs the
# OpenROAD binary (argv0) to launch local workers + a balancer, so it needs the
# baseline + worker + balancer scripts as runfiles. Tagged "manual" and
# "requires-network" because the Bazel sandbox commonly forbids opening
# sockets; it is the CMake/ctest path that gates this test in CI. Run with:
#   bazel test //src/drt/test:gcd_nangate45_distributed_local --test_tag_filters=
filegroup(
    name = "gcd_nangate45_distributed_local_resources",
    srcs = [":regression_resources"] + glob(["gcd_nangate45_distributed_local.*"]),
)

regression_test(
    name = "gcd_nangate45_distributed_local",
    size = "large",
    check_passfail = True,
    data = [":gcd_nangate45_distributed_local_resources"],
    tags = [
        "manual",
        "requires-network",
    ],
    visibility = ["//visibility:public"],
)

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

test_suite(
    name = "gc_test",
    tests = [":gc_unittest"],
)

cc_test(
    name = "gc_unittest",
    srcs = [
        "fixture.cpp",
        "fixture.h",
        "gcTest.cpp",
    ],
    includes = [
        "../src",
    ],
    deps = [
        "//src/drt",  # buildcleaner: keep for private headers
        "//src/drt:base_types",
        "//src/drt:db_hdrs",
        "//src/drt:drt_private_hdrs",
        "//src/odb/src/db",
        "//src/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

test_suite(
    name = "ta_test",
    tests = [":ta_unittest"],
)

cc_test(
    name = "ta_unittest",
    srcs = [
        "fixture.cpp",
        "fixture.h",
        "taTest.cpp",
    ],
    includes = [
        "../src",
    ],
    deps = [
        # drt only exports header TritonRoute.h, but we need other symbols
        "//src/drt",  # buildcleaner: keep
        "//src/drt:base_types",
        "//src/drt:db_hdrs",
        "//src/drt:drt_private_hdrs",
        "//src/odb/src/db",
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
