# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

# L-IR core tests; helpers/ holds the helpers all L-IR tests share.

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

package(
    default_visibility = ["//openroad/syn:__subpackages__"],
    features = ["layering_check"],
)

cc_test(
    name = "ids_test",
    srcs = ["TestIds.cc"],
    deps = [
        "//openroad/syn/src/lir",
        "@abseil-cpp//absl/container:flat_hash_set",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "op_test",
    srcs = ["TestOp.cc"],
    deps = [
        "//openroad/syn/src/lir",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "reg_pin_types_test",
    srcs = ["TestRegPinTypes.cc"],
    deps = [
        "//openroad/syn/src/lir",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "name_interner_test",
    srcs = ["TestNameInterner.cc"],
    deps = [
        "//openroad/syn/src/lir",
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
        "//openroad/syn/src/lir",
        "@abseil-cpp//absl/strings",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "const_pool_test",
    srcs = ["TestConstPool.cc"],
    deps = [
        "//openroad/syn/src/lir",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

# kCheckWellFormed: the DesignOptions every test builds its designs with.
cc_library(
    name = "test_design_options",
    testonly = True,
    hdrs = ["helpers/TestDesignOptions.h"],
    includes = ["."],
    deps = ["//openroad/syn/src/lir"],
)

# addModuleOrFail, and the ModuleCapacity guard.
cc_library(
    name = "test_modules",
    testonly = True,
    srcs = ["helpers/TestModules.cc"],
    hdrs = ["helpers/TestModules.h"],
    includes = ["."],
    deps = [
        "//openroad/syn/src/lir",
        "@abseil-cpp//absl/status:statusor",
        "@googletest//:gtest",
    ],
)

# A small Liberty library whose cells the tests build by hand.
cc_library(
    name = "test_liberty",
    testonly = True,
    srcs = ["helpers/TestLiberty.cc"],
    hdrs = ["helpers/TestLiberty.h"],
    includes = ["."],
    deps = ["//openroad/sta:opensta_lib"],
)

cc_test(
    name = "module_builder_test",
    srcs = ["TestModuleBuilder.cc"],
    deps = [
        ":test_modules",
        "//openroad/syn/src/lir",
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
        "//openroad/syn/src/lir",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "lib_cell_table_test",
    srcs = ["TestLibCellTable.cc"],
    deps = [
        ":test_liberty",
        "//openroad/sta:opensta_lib",
        "//openroad/syn/src/lir",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/strings",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "design_test",
    srcs = ["TestDesign.cc"],
    deps = [
        ":test_design_options",
        ":test_liberty",
        ":test_modules",
        "//openroad/sta:opensta_lib",
        "//openroad/syn/src/lir",
        "//openroad/utl",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

# Fixtures shared by well_formed_test and well_formed_edits_test.
cc_library(
    name = "well_formed_fixtures",
    testonly = True,
    hdrs = ["helpers/WellFormedFixtures.h"],
    includes = ["."],
    deps = [
        ":test_design_options",
        ":test_liberty",
        "//openroad/sta:opensta_lib",
        "//openroad/syn/src/lir",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "well_formed_test",
    srcs = ["TestWellFormed.cc"],
    deps = [
        ":test_design_options",
        ":test_liberty",
        ":test_modules",
        ":well_formed_fixtures",
        "//openroad/sta:opensta_lib",
        "//openroad/syn/src/lir",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
