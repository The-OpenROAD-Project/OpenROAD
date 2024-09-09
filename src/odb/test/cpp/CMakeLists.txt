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
        ${TCL_LIBRARY}
        Boost::boost
        utl_lib
        odb_test_helper
        gdsin
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
)

add_executable(OdbGTests TestDbWire.cc TestAbstractLef.cc)
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
#add_executable(TestXML TestXML.cpp)

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
target_link_libraries(TestGDSIn gdsin odb_test_helper)
#target_link_libraries(TestXML gdsin odb_test_helper)

# FAILING TARGETS
# add_test(NAME TestLef58Properties COMMAND TestLef58Properties)
# add_test(NAME TestJournal COMMAND TestJournal)
# add_test(NAME TestAccessPoint COMMAND TestAccessPoint)

gtest_discover_tests(OdbGTests
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)
add_test(NAME odb.TestCallBacks COMMAND TestCallBacks)
add_test(NAME odb.TestGeom COMMAND TestGeom)
add_test(NAME odb.TestModule COMMAND TestModule)
add_test(NAME odb.TestGroup COMMAND TestGroup)
add_test(NAME odb.TestGCellGrid COMMAND TestGCellGrid)
add_test(NAME odb.TestGuide COMMAND TestGuide)
add_test(NAME odb.TestNetTrack COMMAND TestNetTrack)
add_test(NAME odb.TestMaster COMMAND TestMaster)

add_dependencies(build_and_test 
        TestCallBacks 
        TestGeom 
        TestModule 
        TestGroup 
        TestGCellGrid 
        TestGuide
        TestNetTrack
        TestMaster
        OdbGTests
)
add_subdirectory(helper)
add_subdirectory(scan)
