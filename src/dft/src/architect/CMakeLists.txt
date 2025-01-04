add_library(dft_architect_lib
  # Keep sorted
  Opt.cpp
  ScanArchitect.cpp
  ScanArchitectHeuristic.cpp
  ScanChain.cpp
)

target_link_libraries(dft_architect_lib
  PRIVATE
    dft_base_scan_cell_lib
    dft_clock_domain_lib
    dft_clock_domain_hash_lib
    dft_config_lib
    dft_utils_scan_pin_lib
)

target_link_libraries(dft_architect_lib
  PRIVATE
    odb
)

target_include_directories(dft_architect_lib
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)
