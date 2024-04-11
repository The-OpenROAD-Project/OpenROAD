include("openroad")

set(TEST_NAMES
    upf_test
    levelshifter
)

foreach(TEST_NAME IN LISTS TEST_NAMES)
    or_integration_test("upf" ${TEST_NAME}  ${CMAKE_CURRENT_SOURCE_DIR}/regression)
endforeach()