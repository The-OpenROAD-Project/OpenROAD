# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2024-2025, The OpenROAD Authors

add_library(gdsin
    gdsin.cpp
    gdsUtil.cpp
)

target_include_directories(gdsin
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${TCL_INCLUDE_PATH}
)
target_link_libraries(gdsin
    db
    utl_lib
)

set_target_properties(gdsin
  PROPERTIES
    # python requirement
    POSITION_INDEPENDENT_CODE ON
)

