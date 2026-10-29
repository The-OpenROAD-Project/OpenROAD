# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

include("openroad")

add_library(pex
  src/Model.cpp
  src/ModelBuilder.cpp
  src/CoverMap.cpp
  src/ModelJson.cpp
  src/Patterns.cpp
)

target_include_directories(pex
  PUBLIC
    include
)

target_compile_definitions(pex
  PRIVATE
    BOOST_JSON_HEADER_ONLY
)

target_link_libraries(pex
  PUBLIC
    fsv
  PRIVATE
    utl_lib
    Boost::boost
)
