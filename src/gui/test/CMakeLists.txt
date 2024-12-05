include("openroad")

set(TEST_NAMES
    supported
)

# Skipped
#gui_man_tcl_check
#gui_readme_msgs_check

foreach(TEST_NAME IN LISTS TEST_NAMES)
    or_integration_test("gui" ${TEST_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/regression)
endforeach()
