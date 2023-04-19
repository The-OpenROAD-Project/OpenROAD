
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
