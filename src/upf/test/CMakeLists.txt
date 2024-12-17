include("openroad")

set(TEST_NAMES
    levelshifter
    write
)

# Skipped
#upf_man_tcl_check
#upf_readme_msgs_check

foreach(TEST_NAME IN LISTS TEST_NAMES)
    or_integration_test("upf" ${TEST_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/regression)
endforeach()
