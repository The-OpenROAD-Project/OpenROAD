include("openroad")

set(TEST_NAMES
    check
    flute1
    flute_gcd
    parse_clocks
    pd1
    pd2
    pd_gcd
)

# Skipped
#stt_man_tcl_check
#stt_readme_msgs_check

foreach(TEST_NAME IN LISTS TEST_NAMES)
    or_integration_test("stt" ${TEST_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/regression)
endforeach()
