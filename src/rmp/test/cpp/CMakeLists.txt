include(openroad)

add_executable(RmpGTests TestAbc.cc)
target_link_libraries(RmpGTests
        OpenSTA
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        dbSta_lib
        rsz_lib
        utl_lib
        cut
        rmp_lib
        tst
        ${TCL_LIBRARY}
)

target_include_directories(RmpGTests
    PRIVATE
      ${PROJECT_SOURCE_DIR}/src/rmp/src
)

gtest_discover_tests(RmpGTests
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_dependencies(build_and_test RmpGTests)
