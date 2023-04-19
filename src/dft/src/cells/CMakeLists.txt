add_library(dft_cells_lib
  # Keep sorted
  OneBitScanCell.cpp
  ScanCell.cpp
  ScanCellFactory.cpp
)

target_link_libraries(dft_cells_lib
  PRIVATE
    odb
    dbSta
    OpenSTA
    utl
    dft_clock_domain_lib
    dft_config_lib
    dft_utils_lib
)

target_include_directories(dft_cells_lib
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)
