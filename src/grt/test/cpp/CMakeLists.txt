# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026-2026, The OpenROAD Authors

add_executable(TestCongestionNets TestCongestionNets.cpp)

target_link_libraries(TestCongestionNets
    GTest::gtest
    GTest::gtest_main
    grt_lib
    tst_base
    stt_lib
    utl_lib
)

gtest_discover_tests(TestCongestionNets
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_dependencies(build_and_test TestCongestionNets)

add_executable(TestGuideRestore TestGuideRestore.cpp)

target_link_libraries(TestGuideRestore
    GTest::gtest
    GTest::gtest_main
    grt_lib
    tst
    defin
)

gtest_discover_tests(TestGuideRestore
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_dependencies(build_and_test TestGuideRestore)

add_executable(TestMaze3D TestMaze3D.cpp)

target_link_libraries(TestMaze3D
    GTest::gtest
    GTest::gtest_main
    grt_lib
    tst_base
    stt_lib
    utl_lib
)

gtest_discover_tests(TestMaze3D
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_dependencies(build_and_test TestMaze3D)
