include("openroad")

set(TEST_NAMES
    level3_01
    level3_02
    east_west1
    east_west2
    snap_layer1
)

foreach(TEST_NAME IN LISTS TEST_NAMES)
    or_integration_test("mpl" ${TEST_NAME}  ${CMAKE_CURRENT_SOURCE_DIR}/regression)
endforeach()
