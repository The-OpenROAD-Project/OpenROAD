include("openroad")

set(TEST_NAMES
    partition_gcd
    read_part
)

# Skipped
#par_man_tcl_check
#par_readme_msgs_check

foreach(TEST_NAME IN LISTS TEST_NAMES)
    or_integration_test("par" ${TEST_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/regression)
endforeach()
