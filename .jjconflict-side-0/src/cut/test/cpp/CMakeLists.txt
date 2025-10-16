include(openroad)

add_executable(CutGTests TestAbc.cc)
target_link_libraries(CutGTests
        OpenSTA
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        dbSta_lib
        utl_lib
        ${ABC_LIBRARY}
        cut
        tst
        ${TCL_LIBRARY}
)

target_include_directories(CutGTests
    PRIVATE
      ${PROJECT_SOURCE_DIR}/src/cut/src
)

gtest_discover_tests(CutGTests
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_dependencies(build_and_test CutGTests
)
