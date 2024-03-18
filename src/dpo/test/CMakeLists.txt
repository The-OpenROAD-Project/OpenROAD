include("openroad")

set(TEST_NAMES
  aes
  gcd
  ibex
  multi_height1
  gcd_no_one_site_gaps
  regions1
  regions2
)

foreach(TEST_NAME IN LISTS TEST_NAMES)
  or_integration_test("dpo" ${TEST_NAME}  ${CMAKE_CURRENT_SOURCE_DIR}/regression)
endforeach()
