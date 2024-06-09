include("openroad")

set(TEST_NAMES
    supported
)

foreach(TEST_NAME IN LISTS TEST_NAMES)
    or_integration_test("gui" ${TEST_NAME}  ${CMAKE_CURRENT_SOURCE_DIR}/regression)
endforeach()
