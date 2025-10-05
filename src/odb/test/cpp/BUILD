# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

load("@rules_cc//cc:cc_test.bzl", "cc_test")

package(
    features = ["layering_check"],
)

cc_test(
    name = "TestAbstractLef",
    srcs = [
        "TestAbstractLef.cc",
        "sky130_test_fixture.h",
    ],
    data = [
        "//src/odb/test/data/sky130hd:lef-test-data",
    ],
    deps = [
        "//src/odb",
        "//src/tst",
        "//src/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestAccessPoint",
    srcs = [
        "TestAccessPoint.cpp",
    ],
    deps = [
        "//src/odb",
        "//src/odb/test/cpp/helper",
        "//src/tst",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestCallBacks",
    srcs = [
        "TestCallBacks.cpp",
        "CallBack.h",
    ],
    deps = [
        "//src/odb",
        "//src/odb/test/cpp/helper",
        "//src/tst",
        "@spdlog",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestChips",
    srcs = [
        "TestChips.cpp",
    ],
    deps = [
        "//src/odb",
        "//src/odb/test/cpp/helper",
        "//src/tst",
        "@spdlog",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestDbWire",
    srcs = ["TestDbWire.cc"],
    data = [
        "//src/odb/test/data/sky130hd:lef-test-data",
    ],
    deps = [
        "//src/odb",
        "//src/tst",
        "//src/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestPolygonalFloorplan",
    srcs = [
        "TestPolygonalFloorplan.cc",
        "nangate45_test_fixture.h",
    ],
    data = [
        "//src/odb/test:regression_resources",
    ],
    deps = [
        "//src/odb",
        "//src/tst",
        "//src/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

# TODO: more to come.
