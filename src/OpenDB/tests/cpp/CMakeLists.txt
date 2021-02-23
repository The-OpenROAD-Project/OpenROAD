find_package(Boost)

set(TEST_LIBS
        opendb
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
        utility
)

add_executable( TestCallBacks ${PROJECT_SOURCE_DIR}/tests/cpp/TestCallBacks.cpp )
add_executable( TestGeom ${PROJECT_SOURCE_DIR}/tests/cpp/TestGeom.cpp )
add_executable( TestModule ${PROJECT_SOURCE_DIR}/tests/cpp/TestModule.cpp )
add_executable( TestLef58Properties ${PROJECT_SOURCE_DIR}/tests/cpp/TestLef58Properties.cpp )
add_executable( TestGroup ${PROJECT_SOURCE_DIR}/tests/cpp/TestGroup.cpp )

target_link_libraries(TestCallBacks ${TEST_LIBS})
target_link_libraries(TestGeom ${TEST_LIBS})
target_link_libraries(TestModule ${TEST_LIBS})
target_link_libraries(TestLef58Properties ${TEST_LIBS})
target_link_libraries(TestGroup ${TEST_LIBS})
