add_library(dft_reset_domain_lib
  # Keep sorted
  ResetDomain.cpp
)

target_include_directories(dft_reset_domain_lib
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(dft_reset_domain_lib
  PRIVATE
    odb
    utl_lib
)
