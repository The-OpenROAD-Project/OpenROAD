# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

# L-IR core tests; helpers/ holds the helpers all L-IR tests share.

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

package(
    default_visibility = ["//src/syn:__subpackages__"],
    features = ["layering_check"],
)

# A small Liberty library whose cells the tests build by hand.
cc_library(
    name = "test_liberty",
    testonly = True,
    srcs = ["helpers/TestLiberty.cc"],
    hdrs = ["helpers/TestLiberty.h"],
    includes = ["."],
    deps = ["//src/sta:opensta_lib"],
)

cc_test(
    name = "ids_test",
    srcs = ["TestIds.cc"],
    deps = [
        "//src/syn/src/lir",
        "@abseil-cpp//absl/container:flat_hash_set",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "op_test",
    srcs = ["TestOp.cc"],
    deps = [
        "//src/syn/src/lir",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "reg_pin_types_test",
    srcs = ["TestRegPinTypes.cc"],
    deps = [
        "//src/syn/src/lir",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "name_interner_test",
    srcs = ["TestNameInterner.cc"],
    deps = [
        "//src/syn/src/lir",
        "@abseil-cpp//absl/container:flat_hash_set",
        "@abseil-cpp//absl/strings",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

# The pools' multi-threaded tests; run them with --config=tsan.
cc_test(
    name = "pool_concurrency_test",
    srcs = ["TestPoolConcurrency.cc"],
    tags = ["tsan"],
    deps = [
        "//src/syn/src/lir",
        "@abseil-cpp//absl/strings",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "const_pool_test",
    srcs = ["TestConstPool.cc"],
    deps = [
        "//src/syn/src/lir",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "reg_class_pool_test",
    srcs = ["TestRegClassPool.cc"],
    deps = [
        "//src/syn/src/lir",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "lib_cell_table_test",
    srcs = ["TestLibCellTable.cc"],
    deps = [
        ":test_liberty",
        "//src/sta:opensta_lib",
        "//src/syn/src/lir",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/strings",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
