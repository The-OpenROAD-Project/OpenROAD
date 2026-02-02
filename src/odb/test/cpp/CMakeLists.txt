include(openroad)

set(TEST_LIBS
        odb
        zutil 
        lef
        defin
        defout
        lefin
        lefout
        cdl
        3dblox
        ${TCL_LIBRARY}
        Boost::boost
        utl_lib
        odb_test_helper
        gdsin
        tst
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
)

add_executable(OdbGTests TestDbWire.cc TestDbNet.cpp TestAbstractLef.cc TestPolygonalFloorplan.cc)
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
add_executable(Test3DBloxChecker Test3DBloxChecker.cpp)
add_executable(TestSwapMaster TestSwapMaster.cpp)
add_executable(TestSwapMasterUnusedPort TestSwapMasterUnusedPort.cpp)
add_executable(TestWriteReadDbHier TestWriteReadDbHier.cpp)
add_executable(TestObjectType TestObjectType.cpp)

target_link_libraries(OdbGTests ${TEST_LIBS})
target_link_libraries(TestCallBacks ${TEST_LIBS})
target_link_libraries(TestGeom ${TEST_LIBS})
target_link_libraries(TestModule ${TEST_LIBS})
target_link_libraries(TestLef58Properties ${TEST_LIBS})
target_link_libraries(TestGroup ${TEST_LIBS})
target_link_libraries(TestGCellGrid ${TEST_LIBS})
target_link_libraries(TestJournal ${TEST_LIBS})
target_link_libraries(TestAccessPoint ${TEST_LIBS})
target_link_libraries(TestGuide ${TEST_LIBS})
target_link_libraries(TestNetTrack ${TEST_LIBS})
target_link_libraries(TestMaster ${TEST_LIBS})
target_link_libraries(TestGDSIn ${TEST_LIBS})
target_link_libraries(TestChips ${TEST_LIBS})
target_link_libraries(Test3DBloxParser ${TEST_LIBS})
target_link_libraries(Test3DBloxChecker ${TEST_LIBS})
target_link_libraries(TestSwapMaster ${TEST_LIBS})
target_link_libraries(TestSwapMasterUnusedPort ${TEST_LIBS})
target_link_libraries(TestWriteReadDbHier ${TEST_LIBS})
target_link_libraries(TestObjectType ${TEST_LIBS})

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
        TestSwapMaster
        TestSwapMasterUnusedPort
        TestWriteReadDbHier
        OdbGTests
        TestObjectType
)
add_subdirectory(helper)
add_subdirectory(scan)
