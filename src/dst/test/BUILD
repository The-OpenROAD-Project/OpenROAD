# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2025, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

package(features = ["layering_check"])

cc_library(
    name = "test_helpers",
    hdrs = ["cpp/HelperCallBack.h"],
    deps = ["//src/dst"],
)

cc_test(
    name = "dst_balancer_test",
    srcs = [
        "cpp/TestBalancer.cc",
    ],
    features = ["-layering_check"],  # TODO: includes private headers
    includes = [
        "../src",
    ],

    # TODO(https://github.com/The-OpenROAD-Project/OpenROAD/issues/8599): re-enable once issue is resolved.
    tags = ["manual"],
    deps = [
        ":test_helpers",
        "//src/dst",
        "//src/utl",
        "@boost.asio",
        "@boost.bind",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ] + select({
        "@platforms//os:macos": ["@boost.thread//:thread_mac"],
        "//conditions:default": ["@boost.thread//:thread_posix"],
    }),
)

cc_test(
    name = "dst_distributed_test",
    srcs = [
        "cpp/TestDistributed.cc",
    ],
    includes = [
        "../src",
    ],
    # TODO(https://github.com/The-OpenROAD-Project/OpenROAD/issues/8599): re-enable once issue is resolved.
    tags = ["manual"],
    deps = [
        ":test_helpers",
        "//src/dst",
        "//src/utl",
        "@boost.asio",
        "@boost.bind",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ] + select({
        "@platforms//os:macos": ["@boost.thread//:thread_mac"],
        "//conditions:default": ["@boost.thread//:thread_posix"],
    }),
)

cc_test(
    name = "dst_worker_test",
    srcs = [
        "cpp/TestWorker.cc",
    ],
    features = ["-layering_check"],  # TODO: includes private headers
    includes = [
        "../src",
    ],

    # TODO(https://github.com/The-OpenROAD-Project/OpenROAD/issues/8599): re-enable once issue is resolved.
    tags = ["manual"],
    deps = [
        ":test_helpers",
        "//src/dst",
        "//src/utl",
        "@boost.asio",
        "@boost.bind",
        "@boost.system",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ] + select({
        "@platforms//os:macos": ["@boost.thread//:thread_mac"],
        "//conditions:default": ["@boost.thread//:thread_posix"],
    }),
)
