# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

add_library(syn_elab
  backend_builder.cc
  blackboxes.cc
  driver.cc
)

target_compile_definitions(syn_elab PUBLIC
  SLANG_MUX_LOWERING
  SLANG_NO_YOSYS
)

# Bare headers (backend_builder.h, ir.h, log_stubs.h) — Bazel exposes them via
# includes = ["."], no prefix. Expose to the slang_frontend target too, which
# needs backend_builder.h / ir.h / log_stubs.h at compile time but is declared
# in third-party/ before this directory is processed.
target_include_directories(syn_elab PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(slang_frontend PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# elab/ir.h (picked up via slang_frontend sources) includes syn/ir/Bundle.h, so
# slang_frontend needs syn_ir's staged include dir.
target_link_libraries(slang_frontend PUBLIC yosys_hashlib syn_ir)
# Fold helpers into slang_frontend: their definitions (unimplemented_,
# wire_missing_, transfer_attrs, ir::Net::repeat, etc.) satisfy undefined refs
# in slang_frontend's own sources. Keeping them in syn_elab would create a
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
