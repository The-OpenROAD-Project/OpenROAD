# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("@rules_python//python:defs.bzl", "py_test")
load("//test:regression.bzl", "doc_check_test", "regression_test")

package(features = ["layering_check"])

cc_test(
    name = "TestPatterns",
    srcs = ["cpp/TestPatterns.cpp"],
    data = ["//src/fsv/test:sky130.itf"],
    deps = [
        "//src/fsv:geometry",
        "//src/fsv:stack",
        "//src/pex:patterns",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestModel",
    srcs = ["cpp/TestModel.cpp"],
    deps = [
        "//src/pex:model",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestModelJson",
    srcs = ["cpp/TestModelJson.cpp"],
    deps = [
        "//src/pex:model",
        "//src/pex:model_json",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestModelBuilder",
    size = "medium",
    srcs = ["cpp/TestModelBuilder.cpp"],
    data = ["//src/fsv/test:sky130.itf"],
    deps = [
        "//src/fsv",
        "//src/fsv:geometry",
        "//src/fsv:stack",
        "//src/pex:model",
        "//src/pex:model_builder",
        "//src/pex:model_json",
        "//src/pex:patterns",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestCoverMap",
    srcs = ["cpp/TestCoverMap.cpp"],
    data = ["//src/fsv/test:sky130.itf"],
    deps = [
        "//src/fsv",
        "//src/fsv:stack",
        "//src/pex:cover_map",
        "//src/pex:model",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestIndex",
    srcs = ["cpp/TestIndex.cpp"],
    deps = [
        "//src/odb/src/db",
        "//src/pex:index",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestEvaluator",
    srcs = ["cpp/TestEvaluator.cpp"],
    deps = [
        "//src/fsv:geometry",
        "//src/fsv:stack",
        "//src/odb/src/db",
        "//src/pex:evaluator",
        "//src/pex:model",
        "//src/pex:patterns",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestNetwork",
    srcs = ["cpp/TestNetwork.cpp"],
    data = [
        "//src/fsv/test:sky130.itf",
        "//test:sky130hd_data",
    ],
    deps = [
        "//src/fsv:stack",
        "//src/odb/src/db",
        "//src/pex:network",
        "//src/pex:resistance",
        "//src/tst:db_fixture",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestExtractor",
    size = "medium",
    srcs = ["cpp/TestExtractor.cpp"],
    data = [
        "sky130hd/sky130_fd_sc_hd_merged.lef",
        "sky130hd/sky130hd.tlef",
        "//src/fsv/test:field_solver_gcd.def",
        "//src/fsv/test:sky130.itf",
    ],
    deps = [
        "//src/fsv:stack",
        "//src/odb/src/db",
        "//src/pex:evaluator",
        "//src/pex:extractor",
        "//src/pex:model",
        "//src/pex:network",
        "//src/tst:db_fixture",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestSpef",
    size = "medium",
    srcs = ["cpp/TestSpef.cpp"],
    data = [
        "sky130hd/sky130_fd_sc_hd_merged.lef",
        "sky130hd/sky130hd.tlef",
        "//src/fsv/test:field_solver_gcd.def",
        "//src/fsv/test:sky130.itf",
    ],
    deps = [
        "//src/fsv:stack",
        "//src/odb/src/db",
        "//src/pex:extractor",
        "//src/pex:model",
        "//src/pex:spef_writer",
        "//src/tst:db_fixture",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

EXPERIMENT_DEPS = [
    "//src/pex:evaluator",
    "//src/fsv:net_window",
    "//src/odb/src/db",
    "//src/odb/src/defin",
    "//src/odb/src/lefin",
    "//src/utl",
    "//src/fsv",
    "//src/pex:model_json",
    "//src/fsv:geometry",
    "//src/fsv:stack",
    "//src/pex:cover_map",
    "//src/pex:model",
    "//src/pex:model_builder",
    "//src/pex:patterns",
]

EXPERIMENT_DATA = [
    "sky130hd/sky130_fd_sc_hd_merged.lef",
    "sky130hd/sky130hd.tlef",
    "//src/fsv/test:GT2.itf",
    "//src/fsv/test:field_solver_gcd.def",
    "//src/fsv/test:sky130.itf",
]

# The modeling experiments; see README.md.
cc_binary(
    name = "Experiments",
    testonly = True,
    srcs = ["cpp/Experiments.cpp"],
    data = EXPERIMENT_DATA,
    deps = EXPERIMENT_DEPS,
)

# A short run of every experiment, to keep them building and running.
cc_test(
    name = "Experiments_smoke",
    size = "medium",
    srcs = ["cpp/Experiments.cpp"],
    args = [
        "--smoke",
        "--threads",
        "2",
    ],
    data = EXPERIMENT_DATA,
    deps = EXPERIMENT_DEPS,
)

TESTS = [
    "pex_errors",
    "pex_extract",
]

filegroup(
    name = "test_resources",
    srcs = [
        "helpers.tcl",
        "other_stack.json",
        "sky130_coarse.json",
        "sky130hd/sky130_fd_sc_hd_merged.lef",
        "sky130hd/sky130hd.tlef",
        "sky130hd/sky130hd_tt.lib",
        "version_two.json",
        "//src/fsv/test:field_solver_gcd.def",
        "//src/fsv/test:sky130.itf",
    ] + [test + ".ok" for test in TESTS],
)

[regression_test(
    name = test_name,
    data = [":test_resources"],
) for test_name in TESTS]

py_test(
    name = "pex_man_tcl_check",
    srcs = ["pex_man_tcl_check.py"],
    data = ["//src/pex:doc_files"],
    tags = [
        "doc_check",
        "py",
    ],
    deps = ["//docs/src/scripts:extract_utils"],
)

doc_check_test(
    name = "pex_readme_msgs_check",
    data = [
        "//src/pex:doc_files",
        "//src/pex:messages_txt",
    ],
)
