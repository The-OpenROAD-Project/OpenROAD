# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

include("openroad")

add_executable(TestNeighbors TestNeighbors.cpp)
target_link_libraries(TestNeighbors
  GTest::gtest
  GTest::gtest_main
  stt_lib
  odb
)
target_include_directories(TestNeighbors
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src/stt/src/pdr/src
)
gtest_discover_tests(TestNeighbors
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_dependencies(build_and_test TestNeighbors)

add_executable(TestSteinerTreeBuilder TestSteinerTreeBuilder.cpp)
target_link_libraries(TestSteinerTreeBuilder
  GTest::gtest
  GTest::gtest_main
  stt_lib
  utl_lib
  odb
)
gtest_discover_tests(TestSteinerTreeBuilder
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_dependencies(build_and_test TestSteinerTreeBuilder)
