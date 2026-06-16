# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

add_library(syn_elab
  backend_builder.cc
  blackboxes.cc
  driver.cc
  error.cc
)

target_compile_definitions(syn_elab PUBLIC
  SLANG_MUX_LOWERING
  SLANG_NO_YOSYS
)

# Expose syn_elab and slang_frontend's home directories
# as each other's include paths
target_include_directories(syn_elab PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(slang_frontend PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# elab/ir.h (picked up via slang_frontend sources) includes syn/ir/Bundle.h, so
# slang_frontend needs syn_ir's include directory
target_link_libraries(slang_frontend PUBLIC syn_ir)

# Fold helpers into slang_frontend. Keeping them in syn_elab would create a
# static-link cycle (syn_elab depends on slang_frontend and vice versa).
target_sources(slang_frontend PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/abort_helpers.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/ir.cc
)

target_link_libraries(syn_elab
  PUBLIC
    syn_ir
    slang_frontend
    dbSta_lib
    utl_lib
)
