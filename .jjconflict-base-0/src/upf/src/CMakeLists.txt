# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2025, The OpenROAD Authors

include("openroad")

add_library(upf_lib
  upf.cpp
  writer.cpp
)

target_include_directories(upf_lib
  PUBLIC
    ../include
)

target_link_libraries(upf_lib 
  PRIVATE
    OpenSTA
    dbSta
    odb
    utl_lib
)

swig_lib(NAME      upf
         NAMESPACE upf
         I_FILE    upf.i
         SCRIPTS   upf.tcl
)

target_sources(upf
  PRIVATE
    MakeUpf.cpp
)

messages(
  TARGET upf
  OUTPUT_DIR ..
)

target_link_libraries(upf 
  PRIVATE
    odb
    upf_lib
    utl_lib
)

target_include_directories(upf
  PUBLIC
    ../include
)
