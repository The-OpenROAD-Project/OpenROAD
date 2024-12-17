include("openroad")

set(TEST_NAMES
    drc_test
    ispd18_sample
    ispd18_sample_incr
    ndr_vias1
    ndr_vias2
    obstruction
    single_step
    ta_ap_aligned
    ta_pin_aligned
    top_level_term
    top_level_term2
)

# Skipped
#drt_man_tcl_check
#drt_readme_msgs_check

foreach(TEST_NAME IN LISTS TEST_NAMES)
    or_integration_test("drt" ${TEST_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/regression)
endforeach()
