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

add_library(dft_clock_domain_hash_lib
  # Keep sorted
  ClockDomainHash.cpp
)
target_include_directories(dft_clock_domain_hash_lib
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)
target_link_libraries(dft_clock_domain_hash_lib
  PRIVATE
    utl_lib
    dft_config_lib
    dft_clock_domain_lib
)
