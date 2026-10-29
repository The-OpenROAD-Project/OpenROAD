# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026-2026, The OpenROAD Authors

include("openroad")

or_integration_tests(
  "ret"
  TESTS
    unmap_basic
)

add_executable(TestSequentialUnmapper
  TestSequentialUnmapperCells.cpp
  TestSequentialUnmapperRun.cpp
)

target_include_directories(TestSequentialUnmapper
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(TestSequentialUnmapper
  PRIVATE
    GTest::gtest
    GTest::gtest_main
    ret_lib
    dbSta
    OpenSTA
    tst
    utl_lib
)

gtest_discover_tests(TestSequentialUnmapper
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
