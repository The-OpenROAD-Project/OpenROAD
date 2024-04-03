include("openroad")

set(TEST_NAMES
    macro_only
)

foreach(TEST_NAME IN LISTS TEST_NAMES)
    or_integration_test("mpl2" ${TEST_NAME}  ${CMAKE_CURRENT_SOURCE_DIR}/regression)
endforeach()

add_subdirectory(cpp)
