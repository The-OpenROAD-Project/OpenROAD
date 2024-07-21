# Tests
include("openroad")

set(TEST_NAMES
  one_cell_sky130
  one_cell_nangate45
  sub_modules_sky130
  scan_architect_no_mix_sky130
  scan_architect_clock_mix_sky130
  place_sort_sky130
  max_chain_count_sky130
)

foreach(TEST_NAME IN LISTS TEST_NAMES)
  or_integration_test("dft" ${TEST_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/regression)
endforeach()

find_package(Boost)
add_subdirectory(cpp)
