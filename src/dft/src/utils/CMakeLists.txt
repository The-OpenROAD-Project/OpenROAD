
add_library(dft_utils_lib
  # Keep sorted
  Utils.cpp
)

target_link_libraries(dft_utils_lib
  PRIVATE
   odb
   dbSta 
   OpenSTA
)

target_include_directories(dft_utils_lib
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)


add_library(dft_utils_scan_pin_lib
  ScanPin.cpp
)

target_link_libraries(dft_utils_scan_pin_lib
  PRIVATE
    odb
)

target_include_directories(dft_utils_scan_pin_lib
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)


add_library(dft_utils_formatting_lib INTERFACE)

target_include_directories(dft_utils_formatting_lib
  INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)
