# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors

include("openroad")

add_library(tst
    src/fixture.cpp
)

target_include_directories(tst
  PUBLIC
    include
)

target_link_libraries(tst
  PUBLIC
    dbSta_lib
  PRIVATE
    utl_lib
    odb
    OpenSTA
)
