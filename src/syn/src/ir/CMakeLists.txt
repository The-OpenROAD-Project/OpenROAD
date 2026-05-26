# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

add_library(syn_ir
  Bundle.cc
  Const.cc
  Dump.cc
  Graph.cc
  Instance.cc
  NetTable.cc
  Parse.cc
  TritModel.cc
)

# Re-expose public headers under the "syn/ir/" prefix to mirror the Bazel
# include_prefix/strip_include_prefix setup. configure_file(... COPYONLY)
# stages each header into the build tree and re-copies on source change.
set(_syn_ir_stage ${CMAKE_CURRENT_BINARY_DIR}/include/syn/ir)
foreach(_h Bundle.h Const.h ControlNet.h Graph.h Instance.h Net.h NetTable.h
           NetTableEntry.h TritModel.h)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${_h} ${_syn_ir_stage}/${_h} COPYONLY)
endforeach()

target_include_directories(syn_ir
  PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}/include
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(syn_ir
  PUBLIC
    OpenSTA
    utl_lib
  PRIVATE
    ${ABC_LIBRARY}
)
