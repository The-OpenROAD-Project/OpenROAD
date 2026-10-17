# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

add_executable(TestMinimizeDisplacement TestMinimizeDisplacement.cpp)

target_link_libraries(TestMinimizeDisplacement
    GTest::gtest
    GTest::gtest_main
    ppl
    tst
    defin
)

gtest_discover_tests(TestMinimizeDisplacement
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_dependencies(build_and_test TestMinimizeDisplacement)
