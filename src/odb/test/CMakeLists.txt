include("openroad")

set(TEST_NAMES
    bterm_hier_create
    multi_tech
    transform
    rounding
    sky130hd_multi_patterned
    dont_touch
    import_package
    read_lef
    read_db
    read_zipped
    create_sboxes
    dump_via_rules
    dump_vias
    read_def
    read_def58
    write_def58
    dump_nets
    lef_mask
    write_lef_and_def
    write_lef_polygon
    lef_data_access
    gcd_def_access
    gcd_pdn_def_access
    edit_def
    wire_encoder
    edit_via_params
    row_settings
    db_read_write
    check_routing_tracks
    polygon
    def_parser
    ndr
    gcd_abstract_lef
    gcd_abstract_lef_with_power
    read_abstract_lef
    abstract_origin
    write_macro_placement
    smash_vias
    floorplan_initialize
    replace_design1
    replace_design2
    replace_design3
    design_is_routed1
    design_is_routed2
    design_is_routed3
    design_is_routed_fail1
    design_is_routed_fail2
)

# Skipped
#odb_man_tcl_check
#odb_readme_msgs_check

foreach(TEST_NAME IN LISTS TEST_NAMES)
    or_integration_test("odb" ${TEST_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/regression)
endforeach()

add_subdirectory(cpp)
