or_integration_tests(
  "dft"
  TESTS
    max_chain_count_sky130
    one_cell_sky130
    place_sort_sky130
    scan_architect_clock_mix_sky130
    scan_architect_no_mix_sky130
    scan_architect_register_bank_no_clock_mix_sky130
    scandef_core_sky130
    scandef_sky130
    sub_modules_sky130
)


find_package(Boost CONFIG)
add_subdirectory(cpp)
