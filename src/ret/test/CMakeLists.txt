# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026-2026, The OpenROAD Authors

include("openroad")

or_integration_tests(
  "ret"
  TESTS
    retime_basic
    retime_dsec_deep
    retime_dsec_enable
    retime_dsec_enable_chain
    retime_dsec_existing_names
    retime_dsec_inverting
    retime_dsec_inverting_asap7
    retime_dsec_loop
    retime_dsec_plain
    retime_gated_clock
    retime_hierarchical
    retime_input_limit
    retime_input_path
    retime_latch
    retime_negedge
    retime_no_base_cell
    retime_no_clock
    retime_no_moves
    retime_two_clocks
    retime_unmap
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
