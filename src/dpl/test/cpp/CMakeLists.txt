include(openroad)

add_executable(dpl_test dpl_test.cc)

target_link_libraries(dpl_test
    GTest::gtest
    GTest::gtest_main
    dpl_lib
    tst
)

gtest_discover_tests(dpl_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_executable(placement_density_test placement_density_test.cpp)

target_link_libraries(placement_density_test
    GTest::gtest
    GTest::gtest_main
    dpl_lib
    tst
)

gtest_discover_tests(placement_density_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_dependencies(build_and_test dpl_test placement_density_test)
