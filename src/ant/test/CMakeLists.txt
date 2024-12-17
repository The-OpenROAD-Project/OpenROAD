include("openroad")

set(TEST_NAMES
    ant_check
    ant_report
    check_api1
    check_drt1
    check_grt1
)

# Skipped
#ant_man_tcl_check
#ant_readme_msgs_check

foreach(TEST_NAME IN LISTS TEST_NAMES)
    or_integration_test("ant" ${TEST_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/regression)
endforeach()
