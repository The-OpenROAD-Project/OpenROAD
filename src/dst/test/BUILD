# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2025, The OpenROAD Authors

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
    ],
)
