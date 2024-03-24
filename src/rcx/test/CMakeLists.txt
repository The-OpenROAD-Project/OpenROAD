include("openroad")

set(TEST_NAMES
    generate_pattern
    ext_pattern
    gcd 
    45_gcd
    names
)

foreach(TEST_NAME IN LISTS TEST_NAMES)
    or_integration_test("rcx" ${TEST_NAME}  ${CMAKE_CURRENT_SOURCE_DIR}/regression)
endforeach()