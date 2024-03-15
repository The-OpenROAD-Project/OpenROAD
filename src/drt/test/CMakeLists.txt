include("openroad")

set(TEST_NAMES
    ispd18_sample
    ndr_vias1
    ndr_vias2
    obstruction
    single_step
    ta_ap_aligned
    ta_pin_aligned
    top_level_term
    top_level_term2
    drc_test
)

foreach(TEST_NAME IN LISTS TEST_NAMES)
    or_integration_test("drt" ${TEST_NAME}  ${CMAKE_CURRENT_SOURCE_DIR}/regression)
endforeach()
