# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

package(
    features = ["layering_check"],
)

cc_test(
    name = "TestScanChain",
    srcs = [
        "TestScanChain.cpp",
    ],
    linkstatic = True,  # TODO: remove once deps define all symbols
    deps = [
        "//src/odb",
        "//src/odb/test/cpp/helper",
        "//src/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
