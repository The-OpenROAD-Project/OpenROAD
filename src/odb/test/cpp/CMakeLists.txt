include(openroad)

# Each executable links only the libraries it actually needs to avoid bloated
# binaries. src/odb/test/cpp/BUILD is the reference for the minimal dependency
# set of each test. GTEST_LIBS is the one group shared by every test.
set(GTEST_LIBS
        GTest::gtest
        GTest::gtest_main
)

add_executable(OdbGTests TestDbWire.cc TestDbNet.cpp TestDesignIsRouted.cpp TestAbstractLef.cc TestPolygonalFloorplan.cc)
add_executable(TestCallBacks TestCallBacks.cpp)
add_executable(TestGeom TestGeom.cpp)
add_executable(TestModule TestModule.cpp)
add_executable(TestLef58Properties TestLef58Properties.cpp)
add_executable(TestGroup TestGroup.cpp)
add_executable(TestGCellGrid TestGCellGrid.cpp)
add_executable(TestJournal TestJournal.cpp)
add_executable(TestAccessPoint TestAccessPoint.cpp)
add_executable(TestGuide TestGuide.cpp)
add_executable(TestNetTrack TestNetTrack.cpp)
add_executable(TestMaster TestMaster.cpp)
add_executable(TestGDSIn TestGDSIn.cpp)
add_executable(TestChips TestChips.cpp)
add_executable(Test3DBloxParser Test3DBloxParser.cpp)
add_executable(Test3DBloxChecker Test3DBloxChecker.cpp Test3DBloxCheckerLogicalConn.cpp Test3DBloxCheckerBumps.cpp Test3DBloxCheckerAlignmentMarkers.cpp)
add_executable(Test3DBloxVerilogWriter Test3DBloxVerilogWriter.cpp)
add_executable(TestSwapMaster TestSwapMaster.cpp)
add_executable(TestSwapMasterUnusedPort TestSwapMasterUnusedPort.cpp)
add_executable(TestWriteReadDbHier TestWriteReadDbHier.cpp)
add_executable(TestObjectType TestObjectType.cpp)

target_link_libraries(OdbGTests
        db
        defin
        lefin
        lefout
        OpenSTA
        tst
        utl_lib
        GTest::gmock
        ${GTEST_LIBS}
)
target_link_libraries(TestCallBacks
        db
        odb_test_helper
        tst
        spdlog::spdlog
        ${GTEST_LIBS}
)
target_link_libraries(TestGeom
        db
        ${GTEST_LIBS}
)
target_link_libraries(TestModule
        db
        odb_test_helper
        ${GTEST_LIBS}
)
target_link_libraries(TestLef58Properties
        db
        defin
        defout
        lefin
        lefout
        tst
        ${GTEST_LIBS}
)
target_link_libraries(TestGroup
        db
        odb_test_helper
        ${GTEST_LIBS}
)
target_link_libraries(TestGCellGrid
        db
        odb_test_helper
        ${GTEST_LIBS}
)
target_link_libraries(TestJournal
        db
        odb_test_helper
        ${GTEST_LIBS}
)
target_link_libraries(TestAccessPoint
        db
        odb_test_helper
        ${GTEST_LIBS}
)
target_link_libraries(TestGuide
        db
        odb_test_helper
        ${GTEST_LIBS}
)
target_link_libraries(TestNetTrack
        db
        odb_test_helper
        ${GTEST_LIBS}
)
target_link_libraries(TestMaster
        db
        odb_test_helper
        ${GTEST_LIBS}
)
target_link_libraries(TestGDSIn
        db
        gdsin
        gdsout
        tst
        utl_lib
        ${GTEST_LIBS}
)
target_link_libraries(TestChips
        db
        odb_test_helper
        ${GTEST_LIBS}
)
target_link_libraries(Test3DBloxParser
        3dblox
        db
        odb_test_helper
        tst
        ${GTEST_LIBS}
)
target_link_libraries(Test3DBloxChecker
        3dblox
        db
        tst
        ${GTEST_LIBS}
)
target_link_libraries(Test3DBloxVerilogWriter
        3dblox
        db
        tst
        ${GTEST_LIBS}
)
target_link_libraries(TestSwapMaster
        db
        dbSta_lib
        tst
        ${GTEST_LIBS}
)
target_link_libraries(TestSwapMasterUnusedPort
        db
        dbSta_lib
        tst
        utl_lib
        ${GTEST_LIBS}
)
target_link_libraries(TestWriteReadDbHier
        db
        tst_integrated_fixture
        ${GTEST_LIBS}
)
target_link_libraries(TestObjectType
        db
        tst
        ${GTEST_LIBS}
)

# Skip the tests from being registered here, since they are called via
# cpp_tests.tcl and don't need to be executed twice. The cpp_tests.tcl
# invocation is preferred for TestLef58Properties, TestJournal, and
# TestAccessPoint, since they all write/read from the disk and calling them with
# BASE_DIR set allows us to write to the results dir. Running them outside of
# cpp_tests.tcl results in the output files being written in the current working
# directory.
gtest_discover_tests(OdbGTests
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
    TEST_FILTER "-TestAccessPoint.*,-TestCallBacks.*,-TestGCellGrid.*,-TestGeom.*,-TestGroup.*,-TestGuide.*,-TestJournal.*-TestLef58Properties.*,-TestMaster.*,-TestModule.*,-TestNetTrack.*,-TestChips.*,-Test3DBloxParser.*,-Test3DBloxChecker.*"
)

add_dependencies(build_and_test 
        TestCallBacks 
        TestGeom 
        TestModule 
        TestGroup 
        TestGCellGrid 
        TestGuide
        TestNetTrack
        TestMaster
        TestChips
        Test3DBloxParser
        Test3DBloxChecker
        Test3DBloxVerilogWriter
        TestSwapMaster
        TestSwapMasterUnusedPort
        TestWriteReadDbHier
        OdbGTests
        TestObjectType
)
add_subdirectory(helper)
add_subdirectory(scan)
