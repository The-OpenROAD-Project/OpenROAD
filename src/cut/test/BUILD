# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2025, The OpenROAD Authors

load("@rules_cc//cc:cc_test.bzl", "cc_test")

cc_test(
    name = "cut_abc_test",
    srcs = ["cpp/TestAbc.cc"],
    linkstatic = True,  # TODO: remove once deps define all symbols
    deps = [
        "//src/cut",
        "//src/dbSta",
        "//src/dbSta:dbReadVerilog",
        "//src/tst",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
