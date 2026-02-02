include(openroad)

add_executable(TestHconn TestHconn.cpp)
target_link_libraries(TestHconn
        OpenSTA
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        dbSta_lib
        tst
        utl_lib
        ${TCL_LIBRARY}
)

target_include_directories(TestHconn
    PRIVATE
      ${PROJECT_SOURCE_DIR}/src/dbSta/src
)

gtest_discover_tests(TestHconn
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_dependencies(build_and_test TestHconn
)


add_executable(TestDbSta TestDbSta.cc)
target_link_libraries(TestDbSta
        OpenSTA
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        dbSta_lib
        utl_lib
        rsz_lib
        grt_lib
        dpl_lib
        stt_lib
        odb
        tst
        ${TCL_LIBRARY}
)

target_include_directories(TestDbSta
    PRIVATE
      ${PROJECT_SOURCE_DIR}/src/dbSta/src
)

gtest_discover_tests(TestDbSta
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_dependencies(build_and_test TestDbSta
)

add_executable(TestReadVerilog TestReadVerilog.cpp)
target_link_libraries(TestReadVerilog
        OpenSTA
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        dbSta_lib
        utl_lib
        rsz_lib
        grt_lib
        dpl_lib
        stt_lib
        odb
        tst
        ${TCL_LIBRARY}
)

target_include_directories(TestReadVerilog
    PRIVATE
      ${PROJECT_SOURCE_DIR}/src/dbSta/src
)

gtest_discover_tests(TestReadVerilog
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_dependencies(build_and_test TestReadVerilog
)
