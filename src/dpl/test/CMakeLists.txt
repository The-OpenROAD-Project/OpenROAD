# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023-2025, The OpenROAD Authors

or_integration_tests(
  "dpl"
  TESTS
    aes
    blockage01
    cell_on_block1
    cell_on_block2
    check1
    check2
    check3
    check4
    check5
    check6
    check7
    check8
    check9
    fence01
    fence02
    fence03
    fillers1
    fillers2
    fillers2_verbose
    fillers3
    fillers4
    fillers5
    fillers6
    fillers7
    fillers8
    fillers9
    fillers9_verbose
    fillers10
    fragmented_row01
    fragmented_row02
    fragmented_row03
    fragmented_row04
    gcd
    hybrid_cells
    hybrid_cells2
    ibex
    max_disp1
    mirror1
    mirror2
    mirror3
    multi_height_one_site_gap_disallow
    multi_height_rows
    obstruction1
    obstruction2
    one_site_gap_disallow
    pad01
    pad02
    pad03
    pad04
    pad05
    pad06
    pad07
    pad08
    pad09
    regions1
    regions2
    regions3
    report_failures
    simple01
    simple02
    simple03
    simple04
    simple05
    simple07
    simple08
    simple09
    simple10
    edge_spacing
    # optimization
    aes-opt
    blockage1-opt
    gcd-opt
    gcd_no_one_site_gaps-opt
    gcd_no_one_site_sanity
    ibex-opt
    multi_height1-opt
    edge_spacing-opt
    regions1-opt
    regions2-opt
)

add_executable(dpl_test dpl_test.cc)

target_link_libraries(dpl_test
    GTest::gtest
    GTest::gtest_main
    dpl_lib
    tst
)

gtest_discover_tests(dpl_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_dependencies(build_and_test dpl_test)

