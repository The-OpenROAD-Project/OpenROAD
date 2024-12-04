set(TEST_NAMES
    error1
    get_core_die_areas
    timing_api
    timing_api_2
    timing_api_3
    timing_api_4
    two_designs
    upf_test
    upf_aes
)

foreach(TEST_NAME IN LISTS TEST_NAMES)
    or_integration_test("openroad" ${TEST_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/regression)
endforeach()
