# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023-2025, The OpenROAD Authors

include("openroad")

set(TEST_LIBS
  GTest::gtest
  GTest::gtest_main
  GTest::gmock
  odb
  mpl_lib
  tst
)

add_executable(TestSnapper TestSnapper.cpp)
target_link_libraries(TestSnapper ${TEST_LIBS})
gtest_discover_tests(TestSnapper WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_dependencies(build_and_test
  TestSnapper
)
