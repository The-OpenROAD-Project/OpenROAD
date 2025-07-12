# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2025, The OpenROAD Authors

include("openroad")

project(lext)
add_library(lext)

messages(
TARGET lext
)

target_include_directories(lext
  PUBLIC
    ./include
  PRIVATE
    ./include
)

target_sources(lext
  PRIVATE
    src/blifParser.cpp
    src/blif.cpp
    src/abc_library_factory.cpp
    src/delay_optimization_strategy.cpp
    src/logic_extractor.cpp
    src/logic_cut.cpp
    src/zero_slack_strategy.cpp
)

target_link_libraries(lext
  PRIVATE
    odb
    dbSta
    OpenSTA
    rsz
    dbSta_lib
    libabc
    utl_lib
    ${ABC_LIBRARY}
 )

add_subdirectory(test)
