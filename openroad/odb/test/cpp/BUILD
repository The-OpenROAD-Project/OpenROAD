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
        "//openroad/odb/src/db",
        "//openroad/odb/src/lefout",
        "//openroad/tst:sky130_fixture",
        "//openroad/utl",
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
        "//openroad/odb/src/db",
        "//openroad/odb/test/cpp/helper",
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
        "//openroad/odb/src/db",
        "//openroad/odb/test/cpp/helper",
        "//openroad/tst:db_fixture",
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
        "//openroad/odb/src/db",
        "//openroad/odb/test/cpp/helper",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestDbSet",
    srcs = ["TestDbSet.cpp"],
    deps = [
        "//openroad/odb/src/db",
        "//openroad/odb/test/cpp/helper",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestDbWire",
    srcs = ["TestDbWire.cc"],
    data = [
        "//openroad/odb/test/data/sky130hd:lef-test-data",
    ],
    deps = [
        "//openroad/odb/src/db",
        "//openroad/odb/src/lefin",
        "//openroad/tst",
        "//openroad/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestGCellGrid",
    srcs = ["TestGCellGrid.cpp"],
    deps = [
        "//openroad/odb/src/db",
        "//openroad/odb/test/cpp/helper",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestGCellTileSize",
    srcs = ["TestGCellTileSize.cpp"],
    deps = [
        "//openroad/odb/src/db",
        "//openroad/odb/test/cpp/helper",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestGDSIn",
    srcs = ["TestGDSIn.cpp"],
    data = ["//openroad/odb/test:regression_resources"],
    deps = [
        "//openroad/odb/src/db",
        "//openroad/odb/src/gdsin",
        "//openroad/odb/src/gdsout",
        "//openroad/tst:db_fixture",
        "//openroad/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestGeom",
    srcs = ["TestGeom.cpp"],
    deps = [
        "//openroad/odb/src/db",
        "@boost.polygon",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestGroup",
    srcs = ["TestGroup.cpp"],
    deps = [
        "//openroad/odb/src/db",
        "//openroad/odb/test/cpp/helper",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestGuide",
    srcs = ["TestGuide.cpp"],
    deps = [
        "//openroad/odb/src/db",
        "//openroad/odb/test/cpp/helper",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestInsertBuffer",
    srcs = ["TestInsertBuffer.cpp"],
    data = ["insert_buffer_port_alias.v"],
    deps = [
        "//openroad/odb/src/db",
        "//openroad/tst:db_fixture",
        "//openroad/tst:loaded_design",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestJournal",
    srcs = ["TestJournal.cpp"],
    deps = [
        "//openroad/odb/src/db",
        "//openroad/odb/test/cpp/helper",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestLefIn",
    srcs = ["TestLefIn.cpp"],
    data = ["//test:nangate45_data"],
    deps = [
        "//openroad/odb/src/db",
        "//openroad/odb/src/lefin",
        "//openroad/tst:db_fixture",
        "//openroad/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestLef58Properties",
    srcs = ["TestLef58Properties.cpp"],
    data = ["//openroad/odb/test:regression_resources"],
    deps = [
        "//openroad/odb/src/db",
        "//openroad/odb/src/defin",
        "//openroad/odb/src/defout",
        "//openroad/odb/src/lefin",
        "//openroad/odb/src/lefout",
        "//openroad/tst",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestMaster",
    srcs = ["TestMaster.cpp"],
    deps = [
        "//openroad/odb/src/db",
        "//openroad/odb/test/cpp/helper",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestModule",
    srcs = ["TestModule.cpp"],
    deps = [
        "//openroad/odb/src/db",
        "//openroad/odb/test/cpp/helper",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestNetAbutment",
    srcs = ["TestNetAbutment.cpp"],
    deps = [
        "//openroad/odb/src/db",
        "//openroad/tst:db_fixture",
        "//openroad/tst:physical_builder",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestNetTrack",
    srcs = ["TestNetTrack.cpp"],
    deps = [
        "//openroad/odb/src/db",
        "//openroad/odb/test/cpp/helper",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "OdbGTests",
    srcs = [
        "TestAbstractLef.cc",
        "TestDbNet.cpp",
        "TestDbWire.cc",
        "TestDesignIsRouted.cpp",
        "TestPolygonalFloorplan.cc",
        "TestWireNormalizer.cpp",
    ],
    args = [],
    data = [
        "//openroad/odb/test:regression_resources",
        "//openroad/odb/test/data/sky130hd:lef-test-data",
    ],
    deps = [
        "//openroad/odb/src/db",
        "//openroad/odb/src/defin",
        "//openroad/odb/src/lefin",
        "//openroad/odb/src/lefout",
        "//openroad/odb/src/wireNormalizer",
        "//openroad/sta:opensta_lib",
        "//openroad/tst",
        "//openroad/tst:nangate45_fixture",
        "//openroad/tst:sky130_fixture",
        "//openroad/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestObjectType",
    srcs = ["TestObjectType.cpp"],
    deps = [
        "//openroad/odb/src/db",
        "//openroad/tst:db_fixture",
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
        "//openroad/odb/test:regression_resources",
    ],
    deps = [
        "//openroad/odb/src/db",
        "//openroad/odb/src/defin",
        "//openroad/tst:nangate45_fixture",
        "//openroad/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestShapeDestroy",
    srcs = ["TestShapeDestroy.cpp"],
    deps = [
        "//openroad/odb/src/db",
        "//openroad/odb/test/cpp/helper",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestTechLayerSpacingTable",
    srcs = ["TestTechLayerSpacingTable.cpp"],
    deps = [
        "//openroad/odb/src/db",
        "//openroad/tst:db_fixture",
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
        "//openroad/odb/test:regression_resources",
    ],
    deps = [
        "//openroad/odb/src/3dblox",
        "//openroad/odb/src/db",
        "//openroad/odb/test/cpp/helper",
        "//openroad/tst:db_fixture",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "Test3DBloxChecker",
    srcs = [
        "Test3DBloxChecker.cpp",
        "Test3DBloxCheckerAlignmentMarkers.cpp",
        "Test3DBloxCheckerBumps.cpp",
        "Test3DBloxCheckerFixture.h",
        "Test3DBloxCheckerLogicalConn.cpp",
    ],
    data = [
        "//openroad/odb/test:regression_resources",
    ],
    deps = [
        "//openroad/odb/src/3dblox",
        "//openroad/odb/src/db",
        "//openroad/tst:db_fixture",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "Test3DBloxVerilogWriter",
    srcs = [
        "Test3DBloxVerilogWriter.cpp",
    ],
    data = [
        "//openroad/odb/test:regression_resources",
    ],
    deps = [
        "//openroad/odb/src/3dblox",
        "//openroad/odb/src/db",
        "//openroad/tst:db_fixture",
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
        "//openroad/odb/test:regression_resources",
    ],
    deps = [
        "//openroad/odb/src/db",
        "//openroad/tst:integrated_fixture",
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
        "//openroad/odb/test:regression_resources",
        "//test:nangate45_data",
    ],
    deps = [
        "//openroad/dbSta:dbNetwork",
        "//openroad/odb/src/db",
        "//openroad/tst",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestSwapMasterLeaf",
    srcs = [
        "TestSwapMasterLeaf.cpp",
    ],
    deps = [
        "//openroad/odb/src/db",
        "//openroad/odb/test/cpp/helper",
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
        "//openroad/odb/test:regression_resources",
        "//test:nangate45_data",
    ],
    deps = [
        "//openroad/dbSta:dbNetwork",
        "//openroad/odb/src/db",
        "//openroad/tst",
        "//openroad/utl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "TestDbStream",
    srcs = ["TestDbStream.cpp"],
    deps = [
        "//openroad/odb/src/db",
        "//openroad/tst:db_fixture",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
