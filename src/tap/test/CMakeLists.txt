include("openroad")

set(TEST_NAMES
    disallow_one_site_gaps
    gcd_fakeram
    gcd_nangate45
    gcd_sky130
    gcd_asap7
    invalid_cells
    multiple_calls
    avoid_overlap
    boundary_macros
    gcd_prefix
    gcd_ripup
    no_endcap
    symmetry
    cut_rows
    cut_rows_with_endcaps
    boundary_macros_separate
    boundary_macros_auto_select
    boundary_macros_tapcell
    gcd_sky130_separate
    jpeg_gf180
    aes_gf180
)

foreach(TEST_NAME IN LISTS TEST_NAMES)
    or_integration_test("tap" ${TEST_NAME}  ${CMAKE_CURRENT_SOURCE_DIR}/regression)
endforeach()