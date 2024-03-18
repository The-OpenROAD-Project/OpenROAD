include("openroad")

set(TEST_NAMES
    flute1
    flute_gcd
    check
    parse_clocks
    pd1
    pd2
    pd_gcd
)

foreach(TEST_NAME IN LISTS TEST_NAMES)
    or_integration_test("stt" ${TEST_NAME}  ${CMAKE_CURRENT_SOURCE_DIR}/regression)
endforeach()