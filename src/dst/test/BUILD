# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2025, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

package(features = ["layering_check"])

cc_library(
    name = "test_helpers",
    testonly = True,
    hdrs = ["cpp/HelperCallBack.h"],
    includes = [
        "cpp",
    ],
    deps = ["//src/dst"],
)

cc_test(
    name = "dst_balancer_test",
    srcs = [
        "cpp/TestBalancer.cc",
        "cpp/stubs.cpp",
    ],
    features = ["-layering_check"],  # TODO: includes private headers
    includes = [
        "../src",
    ],
    deps = [
        ":test_helpers",
        "//:ord",
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

# Re-enabled as a cc_test: the former race was unsynchronized async startup +
# hard-coded ports (see DIST_MULTIHOST_INVESTIGATION.md Part B). TestDistributed
# now allocates ephemeral ports and polls for listener readiness before sending.
cc_test(
    name = "dst_distributed_test",
    srcs = [
        "cpp/TestDistributed.cc",
        "cpp/stubs.cpp",
    ],
    includes = [
        "../src",
    ],
    deps = [
        ":test_helpers",
        "//:ord",
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
        "cpp/stubs.cpp",
    ],
    features = ["-layering_check"],  # TODO: includes private headers
    includes = [
        "../src",
    ],
    deps = [
        ":test_helpers",
        "//:ord",
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

test_suite(
    name = "dst.TestWorker",
    tests = [":dst_worker_test"],
)

test_suite(
    name = "dst.TestBalancer",
    tests = [":dst_balancer_test"],
)

test_suite(
    name = "dst.TestDistributed",
    tests = [":dst_distributed_test"],
)
