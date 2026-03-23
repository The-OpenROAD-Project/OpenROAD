# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//:__subpackages__"],
    features = ["layering_check"],
)

cc_library(
    name = "nangate45_fixture",
    testonly = 1,
    hdrs = ["include/tst/nangate45_fixture.h"],
    data = [
        "//test:nangate45_data",
    ],
    includes = ["."],
    visibility = ["//visibility:public"],
    deps = [":tst"],
)

cc_library(
    name = "sky130_fixture",
    testonly = 1,
    hdrs = ["include/tst/sky130_fixture.h"],
    data = [
        "//test:sky130hd_data",
    ],
    includes = ["."],
    visibility = ["//visibility:public"],
    deps = [":tst"],
)

cc_library(
    name = "tst",
    testonly = 1,
    srcs = [
        "src/fixture.cpp",
    ],
    hdrs = [
        "include/tst/fixture.h",
    ],
    includes = [
        "include",
    ],
    deps = [
        "//src/dbSta",
        "//src/dbSta:dbNetwork",
        "//src/odb",
        "//src/sta:opensta_lib",
        "//src/utl",
        "@bazel_tools//tools/cpp/runfiles",
        "@googletest//:gtest",
        "@spdlog",
        "@tcl_lang//:tcl",
    ],
)

cc_library(
    name = "integrated_fixture",
    testonly = 1,
    srcs = ["src/IntegratedFixture.cpp"],
    hdrs = ["include/tst/IntegratedFixture.h"],
    data = [
        "//test:nangate45_data",
        "//test:sky130hd_data",
    ],
    includes = ["."],
    visibility = ["//visibility:public"],
    deps = [
        ":tst",
        "//src/ant",
        "//src/dbSta",
        "//src/dbSta:dbNetwork",
        "//src/dbSta:dbReadVerilog",
        "//src/dpl",
        "//src/est",
        "//src/grt",
        "//src/odb",
        "//src/rsz",
        "//src/sta:opensta_lib",
        "//src/stt",
        "//src/utl",
        "@bazel_tools//tools/cpp/runfiles",
        "@googletest//:gtest",
        "@spdlog",
        "@tcl_lang//:tcl",
    ],
)
