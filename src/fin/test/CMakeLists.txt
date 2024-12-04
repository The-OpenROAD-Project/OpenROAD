include("openroad")

set(TEST_NAMES
    gcd_fill
)

# Skipped
#fin_man_tcl_check
#fin_readme_msgs_check

foreach(TEST_NAME IN LISTS TEST_NAMES)
    or_integration_test("fin" ${TEST_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/regression)
endforeach()
