add_library(dft_clock_domain_lib
  # Keep sorted
  ClockDomain.cpp
)

target_include_directories(dft_clock_domain_lib
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(dft_clock_domain_lib
  PRIVATE
    utl_lib
    dft_config_lib
)
