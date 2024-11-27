include("openroad")

set(TEST_NAMES
    read_part
    partition_gcd
)

foreach(TEST_NAME IN LISTS TEST_NAMES)
    or_integration_test("par" ${TEST_NAME}  ${CMAKE_CURRENT_SOURCE_DIR}/regression)
endforeach()
