# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors

add_library(dft_optimizer_lib
  # Keep sorted
  OdbScanCellAdapter.cpp
  Opt.cpp
)

target_link_libraries(dft_optimizer_lib
  PRIVATE
    dft_base_scan_cell_lib
    dft_clock_domain_lib
    dft_utils_scan_pin_lib
)

target_link_libraries(dft_optimizer_lib
  PRIVATE
    odb
)

target_include_directories(dft_optimizer_lib
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)
