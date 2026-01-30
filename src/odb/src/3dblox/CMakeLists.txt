# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2025, The OpenROAD Authors

find_package(yaml-cpp REQUIRED)

add_library(3dblox
  baseParser.cpp
  bmapParser.cpp
  dbvParser.cpp
  dbxParser.cpp
  baseWriter.cpp
  bmapWriter.cpp
  dbvWriter.cpp
  dbxWriter.cpp
  3dblox.cpp
  unfoldedModel.cpp
  checker.cpp
)

target_link_libraries(3dblox
  PUBLIC
    db
    utl_lib
    yaml-cpp
    OpenSTA
)

target_include_directories(3dblox
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${TCL_INCLUDE_PATH}
)
