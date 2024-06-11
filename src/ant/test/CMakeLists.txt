include("openroad")

set(TEST_NAMES
  check_api1
  check_drt1
  check_grt1
  ant_check
  ant_report
)

foreach(TEST_NAME IN LISTS TEST_NAMES)
  or_integration_test("ant" ${TEST_NAME}  ${CMAKE_CURRENT_SOURCE_DIR}/regression)
endforeach()