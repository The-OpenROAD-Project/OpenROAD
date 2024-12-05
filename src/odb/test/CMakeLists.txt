include("openroad")

set(TEST_NAMES
    abstract_origin
    bterm_hier_create
    check_routing_tracks
    create_sboxes
    db_read_write
    def_parser
    design_is_routed1
    design_is_routed2
    design_is_routed3
    design_is_routed_fail1
    design_is_routed_fail2
    dont_touch
    dump_nets
    dump_via_rules
    dump_vias
    edit_def
    edit_via_params
    floorplan_initialize
    gcd_abstract_lef
    gcd_abstract_lef_with_power
    gcd_def_access
    gcd_pdn_def_access
    import_package
    lef_data_access
    lef_mask
    multi_tech
    ndr
    polygon
    read_abstract_lef
    read_db
    read_def
    read_def58
    read_lef
    read_zipped
    replace_design1
    replace_design2
    replace_design3
    rounding
    row_settings
    sky130hd_multi_patterned
    smash_vias
    transform
    wire_encoder
    write_def58
    write_lef_and_def
    write_lef_polygon
    write_macro_placement
)

# Skipped
#odb_man_tcl_check
#odb_readme_msgs_check

foreach(TEST_NAME IN LISTS TEST_NAMES)
    or_integration_test("odb" ${TEST_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/regression)
endforeach()

add_subdirectory(cpp)
