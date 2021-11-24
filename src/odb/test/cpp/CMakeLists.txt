find_package(Boost)

set(TEST_LIBS
        odb
        zutil 
        zlib 
        tm
        lef
        defin
        defout
        lefout
        lefin
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

target_link_libraries(TestCallBacks ${TEST_LIBS})
target_link_libraries(TestGeom ${TEST_LIBS})
target_link_libraries(TestModule ${TEST_LIBS})
target_link_libraries(TestLef58Properties ${TEST_LIBS})
target_link_libraries(TestGroup ${TEST_LIBS})
target_link_libraries(TestGCellGrid ${TEST_LIBS})
target_link_libraries(TestJournal ${TEST_LIBS})
