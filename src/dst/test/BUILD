# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2025, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_library(
    name = "test_helpers",
    hdrs = ["cpp/HelperCallBack.h"],
)

cc_test(
    name = "dst_balancer_test",
    srcs = [
        "cpp/TestBalancer.cc",
    ],
    includes = [
        "../src",
    ],
    linkstatic = True,  # TODO: remove once deps define all symbols
    deps = [
        ":test_helpers",
        "//src/dst",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "dst_distributed_test",
    srcs = [
        "cpp/TestDistributed.cc",
    ],
    includes = [
        "../src",
    ],
    linkstatic = True,  # TODO: remove once deps define all symbols
    deps = [
        ":test_helpers",
        "//src/dst",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "dst_worker_test",
    srcs = [
        "cpp/TestWorker.cc",
    ],
    includes = [
        "../src",
    ],
    linkstatic = True,  # TODO: remove once deps define all symbols
    deps = [
        ":test_helpers",
        "//src/dst",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
