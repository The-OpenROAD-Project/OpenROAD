# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2025, The OpenROAD Authors

load("@rules_cc//cc:cc_test.bzl", "cc_test")

package(features = ["layering_check"])

cc_test(
    name = "cut_abc_test",
    srcs = ["cpp/TestAbc.cc"],
    linkstatic = True,  # TODO: remove once deps define all symbols
    deps = [
        "//src/cut",
        "//src/dbSta",
        "//src/dbSta:dbReadVerilog",
        "//src/odb",
        "//src/sta:opensta_lib",
        "//src/tst",
        "//src/utl",
        "@edu_berkeley_abc//:abc-lib",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
        "@tk_tcl//:tcl",
    ],
)
