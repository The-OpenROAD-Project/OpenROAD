# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

package(features = ["layering_check"])

cc_test(
    name = "TestPatterns",
    srcs = ["cpp/TestPatterns.cpp"],
    data = ["//openroad/fsv/test:sky130.itf"],
    deps = [
        "//openroad/fsv:geometry",
        "//openroad/fsv:stack",
        "//openroad/pex:patterns",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestModel",
    srcs = ["cpp/TestModel.cpp"],
    deps = [
        "//openroad/pex:model",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestModelJson",
    srcs = ["cpp/TestModelJson.cpp"],
    deps = [
        "//openroad/pex:model",
        "//openroad/pex:model_json",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestModelBuilder",
    size = "medium",
    srcs = ["cpp/TestModelBuilder.cpp"],
    data = ["//openroad/fsv/test:sky130.itf"],
    deps = [
        "//openroad/fsv",
        "//openroad/fsv:geometry",
        "//openroad/fsv:stack",
        "//openroad/pex:model",
        "//openroad/pex:model_builder",
        "//openroad/pex:model_json",
        "//openroad/pex:patterns",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestCoverMap",
    srcs = ["cpp/TestCoverMap.cpp"],
    data = ["//openroad/fsv/test:sky130.itf"],
    deps = [
        "//openroad/fsv",
        "//openroad/fsv:stack",
        "//openroad/pex:cover_map",
        "//openroad/pex:model",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestIndex",
    srcs = ["cpp/TestIndex.cpp"],
    deps = [
        "//openroad/odb/src/db",
        "//openroad/pex:index",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestEvaluator",
    srcs = ["cpp/TestEvaluator.cpp"],
    deps = [
        "//openroad/fsv:geometry",
        "//openroad/fsv:stack",
        "//openroad/odb/src/db",
        "//openroad/pex:evaluator",
        "//openroad/pex:model",
        "//openroad/pex:patterns",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

EXPERIMENT_DEPS = [
    "//openroad/pex:evaluator",
    "//openroad/fsv:net_window",
    "//openroad/odb/src/db",
    "//openroad/odb/src/defin",
    "//openroad/odb/src/lefin",
    "//openroad/utl",
    "//openroad/fsv",
    "//openroad/pex:model_json",
    "//openroad/fsv:geometry",
    "//openroad/fsv:stack",
    "//openroad/pex:cover_map",
    "//openroad/pex:model",
    "//openroad/pex:model_builder",
    "//openroad/pex:patterns",
]

EXPERIMENT_DATA = [
    "sky130hd/sky130_fd_sc_hd_merged.lef",
    "sky130hd/sky130hd.tlef",
    "//openroad/fsv/test:GT2.itf",
    "//openroad/fsv/test:field_solver_gcd.def",
    "//openroad/fsv/test:sky130.itf",
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
