add_library(dft_cells_lib
  # Keep sorted
  OneBitScanCell.cpp
  ScanCellFactory.cpp
)

target_link_libraries(dft_cells_lib
  PRIVATE
    odb
    dbSta
    OpenSTA
    utl_lib
    dft_clock_domain_lib
    dft_config_lib
    dft_utils_lib
)

target_include_directories(dft_cells_lib
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

add_library(dft_base_scan_cell_lib
  ScanCell.cpp
)

target_link_libraries(dft_base_scan_cell_lib
  PRIVATE
    odb
    dft_clock_domain_lib
    dft_config_lib
    dft_utils_scan_pin_lib
)

target_include_directories(dft_base_scan_cell_lib
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)
