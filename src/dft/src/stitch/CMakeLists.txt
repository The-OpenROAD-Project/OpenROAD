add_library(dft_stitch_lib
  # Keep sorted
  ScanStitch.cpp
)

target_include_directories(dft_stitch_lib
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(dft_stitch_lib
  PRIVATE
    odb
    dbSta
    OpenSTA
    utl_lib
    dft_cells_lib
    dft_architect_lib
    dft_utils_lib
    dft_config_lib
)
