# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2025, The OpenROAD Authors

include("openroad")

project(cut)
add_library(cut)

messages(
TARGET cut
)

target_include_directories(cut
  PUBLIC
    ../include
)

target_sources(cut
  PRIVATE
    blifParser.cpp
    blif.cpp
    abc_library_factory.cpp
    logic_extractor.cpp
    logic_cut.cpp
)

target_link_libraries(cut
  PUBLIC
    rsz_lib
    ${ABC_LIBRARY}
  PRIVATE
    odb
    dbSta
    OpenSTA
    dbSta_lib
    utl_lib
 )
