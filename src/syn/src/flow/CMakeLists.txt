# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

add_library(syn_flow
  abc.cc
  bitblast.cc
  combinational_mapper.cc
  combinational_mapper_npn.cc
  export.cc
  import.cc
  liveness.cc
  constant_fold.cc
  opt_gatefusion.cc
  sequential_mapper.cc
)

set(_syn_flow_stage ${CMAKE_CURRENT_BINARY_DIR}/include/flow)
foreach(_h combinational_mapper_npn.h export.h import.h opt_gatefusion.h)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${_h} ${_syn_flow_stage}/${_h} COPYONLY)
endforeach()

target_include_directories(syn_flow
  PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

target_link_libraries(syn_flow
  PUBLIC
    syn_ir
    dbSta_lib
    odb
    OpenSTA
    utl_lib
    rsz_lib
  PRIVATE
    ${ABC_LIBRARY}
)
