# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("@rules_cc//cc:cc_test.bzl", "cc_test")

package(
    features = ["layering_check"],
)

cc_test(
    name = "TestScanChain",
    srcs = [
        "TestScanChain.cpp",
    ],
    deps = [
        "//src/odb/src/db",
        "//src/odb/test/cpp/helper",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
