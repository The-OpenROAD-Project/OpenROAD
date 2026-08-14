# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

include("openroad")

swig_lib(NAME      wmk
         NAMESPACE wmk
         I_FILE    src/watermark.i
         SCRIPTS   src/watermark.tcl
)

target_sources(wmk
  PRIVATE
    src/Watermark.cpp
    src/MakeWatermark.cpp
    src/HmacSha256.cpp
)

target_include_directories(wmk
  PUBLIC
    include
  PRIVATE
    src
)

target_link_libraries(wmk
  PRIVATE
    ${TCL_LIBRARY}
    odb
    utl_lib
)

messages(
  TARGET wmk
)
