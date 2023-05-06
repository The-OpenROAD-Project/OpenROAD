add_library(dft_architect_lib
  # Keep sorted
  ScanArchitect.cpp
  ScanArchitectHeuristic.cpp
  ScanChain.cpp
)

target_link_libraries(dft_architect_lib
  PRIVATE
    utl
    dft_utils_lib
    dft_cells_lib
    dft_clock_domain_lib
    dft_config_lib
)

target_link_libraries(dft_architect_lib
  PRIVATE
    odb
    dbSta
    OpenSTA
    utl
)

target_include_directories(dft_architect_lib
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)
