include("openroad")

set(TEST_NAMES
    45_gcd
    ext_pattern
    gcd
    generate_pattern
    names
)

# Skipped
#generate_rules
#rcx_man_tcl_check
#rcx_readme_msgs_check

foreach(TEST_NAME IN LISTS TEST_NAMES)
    or_integration_test("rcx" ${TEST_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/regression)
endforeach()
