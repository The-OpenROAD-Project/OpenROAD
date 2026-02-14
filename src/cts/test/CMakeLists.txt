# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023-2025, The OpenROAD Authors

or_integration_tests(
  "cts"
  TESTS
    array
    array_ins_delay
    array_max_wl
    array_no_blockages
    array_repair_clock_nets
    check_buffers
    check_buffers_blockages
    check_buffers_blockages_merge
    check_buffer_inference1
    check_buffer_inference2
    check_buffer_inference3
    check_charBuf
    check_max_fanout1
    check_max_fanout2
    check_max_fanout3
    check_wire_rc_cts
    clock_buffer_footprint
    dummy_load
    find_clock
    find_clock_pad
    gated_clock1
    gated_clock2
    gated_clock3
    gated_clock4
    gated_clock5
    hier_insertion_delay
    insertion_delay
    inverters
    lvt_lib
    max_cap
    no_clocks
    no_sinks
    post_cts_opt
    simple_test
    simple_test_clustered
    simple_test_clustered_max_cap
    simple_test_hier
    sink_clustering_collision
    skip_nets
    twice
)

add_executable(cts_unittest cts_unittest.cc)
target_include_directories(cts_unittest
  PUBLIC
    ${OPENROAD_HOME}
)

target_link_libraries(cts_unittest
    GTest::gtest
    GTest::gtest_main
    cts_lib
    utl_lib
    dbSta_lib
    OpenSTA
    odb
)

gtest_discover_tests(cts_unittest
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_dependencies(build_and_test cts_unittest)

