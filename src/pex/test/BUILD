# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

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
