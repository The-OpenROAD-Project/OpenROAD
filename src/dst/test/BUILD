# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2025, The OpenROAD Authors

load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_test(
    name = "dst_balancer_test",
    srcs = [
        "cpp/HelperCallBack.h",
        "cpp/TestBalancer.cc",
    ],
    includes = [
        "../src",
    ],
    linkstatic = True,  # TODO: remove once deps define all symbols
    deps = [
        "//src/dst",
        "@boost.test",
    ],
)

cc_test(
    name = "dst_distributed_test",
    srcs = [
        "cpp/HelperCallBack.h",
        "cpp/TestDistributed.cc",
    ],
    includes = [
        "../src",
    ],
    linkstatic = True,  # TODO: remove once deps define all symbols
    deps = [
        "//src/dst",
        "@boost.test",
    ],
)

cc_test(
    name = "dst_worker_test",
    srcs = [
        "cpp/HelperCallBack.h",
        "cpp/TestWorker.cc",
    ],
    includes = [
        "../src",
    ],
    linkstatic = True,  # TODO: remove once deps define all symbols
    deps = [
        "//src/dst",
        "@boost.test",
    ],
)
