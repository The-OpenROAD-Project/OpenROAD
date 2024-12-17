include("openroad")

set(TEST_NAMES
    east_west1
    east_west2
    level3_01
    level3_02
    snap_layer1
)

# Skipped
#mpl_man_tcl_check
#mpl_readme_msgs_check

foreach(TEST_NAME IN LISTS TEST_NAMES)
    or_integration_test("mpl" ${TEST_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/regression)
endforeach()
