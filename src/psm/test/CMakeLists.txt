include("openroad")

set(TEST_NAMES
    aes_test_vdd
    aes_test_vss
    gcd_test_vdd
    gcd_no_vsrc
    gcd_write_sp_test_vdd
    gcd_all_vss
    gcd_em_test_vdd
    gcd_vss_no_vsrc
    gcd_sky130_vdd
    aes_asap7_vdd
    check_power_grid
    check_power_grid_floorplanning
    check_power_grid_disconnected
    check_power_grid_ok_disconnected
    check_power_grid_macros
    check_power_grid_disconnected_macro
    corners
    aes_test_bterms
    aes_test_multiple_bterms
    zerosoc_pads
    zerosoc_pads_check_only
    zerosoc_pads_check_only_disconnected
    pad_connected_by_abutment
    switch_top_grid
    top_grid_settings
    insert_decap1
    insert_decap2
    insert_decap_with_padding1
    #psm_man_tcl_check
    #psm_readme_msgs_check
)

foreach(TEST_NAME IN LISTS TEST_NAMES)
    or_integration_test("psm" ${TEST_NAME}  ${CMAKE_CURRENT_SOURCE_DIR}/regression)
endforeach()
