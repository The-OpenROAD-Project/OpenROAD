# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2025, The OpenROAD Authors

add_library(FastRoute4.1
  src/FastRoute.cpp
  src/RSMT.cpp
  src/RipUp.cpp
  src/maze.cpp
  src/maze3D.cpp
  src/route.cpp
  src/utility.cpp
)

target_compile_options(FastRoute4.1
  PRIVATE
    -Wno-array-bounds
)

target_include_directories(FastRoute4.1
  PUBLIC
    include
  PRIVATE
    # Ugly but necessary to get GRoute.h and avoid a circular dependency
    # with FastRoute.  Remove once this data moves to OpenDB
    ../../include
  )

target_link_libraries(FastRoute4.1
  PRIVATE
    utl_lib
    stt_lib
    odb
    Boost::boost
)
