add_library(dft_config_lib
  # Keep sorted
  DftConfig.cpp
  ScanArchitectConfig.cpp
)

target_include_directories(dft_config_lib
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(dft_config_lib
  PRIVATE
    odb
    utl_lib
    dft_utils_formatting_lib
)
