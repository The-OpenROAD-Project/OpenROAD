include(openroad)

add_executable(LextGTests TestAbc.cc)
target_link_libraries(LextGTests
        OpenSTA
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        dbSta_lib
        utl_lib
        libabc
        lext
        ${TCL_LIBRARY}
)

target_include_directories(LextGTests
    PRIVATE
      ${PROJECT_SOURCE_DIR}/src/lext/src
)

gtest_discover_tests(LextGTests
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_dependencies(build_and_test LextGTests
)
