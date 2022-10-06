find_package(Boost)

set(TEST_LIBS
        odb
        zutil 
        tm
        lef
        defin
        defout
        lefin
        lefout
        cdl
        ${TCL_LIBRARY}
        Boost::boost
        utl
)

add_executable(TestCallBacks TestCallBacks.cpp)
add_executable(TestGeom TestGeom.cpp)
add_executable(TestModule TestModule.cpp)
add_executable(TestLef58Properties TestLef58Properties.cpp)
add_executable(TestGroup TestGroup.cpp)
add_executable(TestGCellGrid TestGCellGrid.cpp)
add_executable(TestJournal TestJournal.cpp)
add_executable(TestAccessPoint TestAccessPoint.cpp)
add_executable(TestGuide TestGuide.cpp)

target_link_libraries(TestCallBacks ${TEST_LIBS})
target_link_libraries(TestGeom ${TEST_LIBS})
target_link_libraries(TestModule ${TEST_LIBS})
target_link_libraries(TestLef58Properties ${TEST_LIBS})
target_link_libraries(TestGroup ${TEST_LIBS})
target_link_libraries(TestGCellGrid ${TEST_LIBS})
target_link_libraries(TestJournal ${TEST_LIBS})
target_link_libraries(TestAccessPoint ${TEST_LIBS})
target_link_libraries(TestGuide ${TEST_LIBS})
