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
    ],
    deps = [
        "//src/odb",
        "//src/tst",
        "//src/tst:sky130_fixture",
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
        "CallBack.h",
        "TestCallBacks.cpp",
    ],
    deps = [
        "//src/odb",
        "//src/odb/test/cpp/helper",
        "//src/tst",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
        "@spdlog",
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
        "@googletest//:gtest",
        "@googletest//:gtest_main",
        "@spdlog",
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
    name = "TestGCellGrid",
    srcs = ["TestGCellGrid.cpp"],
    deps = [
        "//src/odb",
        "//src/odb/test/cpp/helper",
        "//src/tst",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestGDSIn",
    srcs = ["TestGDSIn.cpp"],
    data = ["//src/odb/test:regression_resources"],
    deps = [
        "//src/odb",
        "//src/odb/test/cpp/helper",
        "//src/tst",
        "//src/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestGeom",
    srcs = ["TestGeom.cpp"],
    deps = [
        "//src/odb",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestGroup",
    srcs = ["TestGroup.cpp"],
    deps = [
        "//src/odb",
        "//src/odb/test/cpp/helper",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestGuide",
    srcs = ["TestGuide.cpp"],
    deps = [
        "//src/odb",
        "//src/odb/test/cpp/helper",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestJournal",
    srcs = ["TestJournal.cpp"],
    deps = [
        "//src/odb",
        "//src/odb/test/cpp/helper",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestLef58Properties",
    srcs = ["TestLef58Properties.cpp"],
    data = ["//src/odb/test:regression_resources"],
    deps = [
        "//src/odb",
        "//src/tst",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestMaster",
    srcs = ["TestMaster.cpp"],
    deps = [
        "//src/odb",
        "//src/odb/test/cpp/helper",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestModule",
    srcs = ["TestModule.cpp"],
    deps = [
        "//src/odb",
        "//src/odb/test/cpp/helper",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestNetTrack",
    srcs = ["TestNetTrack.cpp"],
    deps = [
        "//src/odb",
        "//src/odb/test/cpp/helper",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestPolygonalFloorplan",
    srcs = [
        "TestPolygonalFloorplan.cc",
    ],
    data = [
        "//src/odb/test:regression_resources",
    ],
    deps = [
        "//src/odb",
        "//src/tst",
        "//src/tst:nangate45_fixture",
        "//src/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "Test3DBloxParser",
    srcs = [
        "Test3DBloxParser.cpp",
    ],
    data = [
        "//src/odb/test:regression_resources",
    ],
    deps = [
        "//src/odb",
        "//src/odb/test/cpp/helper",
        "//src/tst",
        "//src/tst:nangate45_fixture",
        "//src/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "Test3DBloxChecker",
    srcs = [
        "Test3DBloxChecker.cpp",
    ],
    data = [
        "//src/odb/test:regression_resources",
    ],
    deps = [
        "//src/odb",
        "//src/tst",
        "//src/tst:nangate45_fixture",
        "//src/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestWriteReadDbHier",
    srcs = [
        "TestWriteReadDbHier.cpp",
    ],
    data = [
        "//src/odb/test:regression_resources",
    ],
    deps = [
        "//src/dbSta",
        "//src/dbSta:dbNetwork",
        "//src/odb",
        "//src/sta:opensta_lib",
        "//src/tst",
        "//src/tst:integrated_fixture",
        "//src/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestSwapMaster",
    srcs = [
        "TestSwapMaster.cpp",
    ],
    data = [
        "//src/odb/test:regression_resources",
    ],
    deps = [
        "//src/dbSta:dbNetwork",
        "//src/odb",
        "//src/tst",
        "//src/tst:integrated_fixture",
        "//src/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestSwapMasterUnusedPort",
    srcs = [
        "TestSwapMasterUnusedPort.cpp",
    ],
    data = [
        "//src/odb/test:regression_resources",
    ],
    deps = [
        "//src/dbSta:dbNetwork",
        "//src/odb",
        "//src/tst",
        "//src/tst:integrated_fixture",
        "//src/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
