# Copyright 2023 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

or_integration_tests(
  "cts"
  TESTS
    array
    array_ins_delay
    array_no_blockages
    balance_levels
    check_buffers
    check_buffers_blockages
    check_buffers_blockages_merge
    check_charBuf
    check_max_fanout1
    check_max_fanout2
    check_wire_rc_cts
    dummy_load
    find_clock
    find_clock_pad
    insertion_delay
    lvt_lib
    max_cap
    no_clocks
    no_sinks
    post_cts_opt
    simple_test
    simple_test_clustered
    simple_test_clustered_max_cap
    simple_test_hier
    twice
)

# Skipped
#cts_man_tcl_check
#cts_readme_msgs_check
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

