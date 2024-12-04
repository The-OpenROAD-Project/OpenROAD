include("openroad")

set(TEST_NAMES
    aes
    blockage1
    gcd
    gcd_no_one_site_gaps
    ibex
    multi_height1
    regions1
    regions2
)

foreach(TEST_NAME IN LISTS TEST_NAMES)
    or_integration_test("dpo" ${TEST_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/regression)
endforeach()
