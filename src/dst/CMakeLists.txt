# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021-2025, The OpenROAD Authors

include("openroad")
include(FindZLIB)
find_package(TCL)

project(dst
  LANGUAGES CXX
)

find_package(Boost CONFIG REQUIRED COMPONENTS serialization thread)
swig_lib(NAME      dst
         NAMESPACE dst
         I_FILE    src/Distributed.i
         SCRIPTS   src/Distributed.tcl
)

add_library(dst_lib
  src/JobMessage.cc
  src/Worker.cc
  src/LoadBalancer.cc
  src/WorkerConnection.cc
  src/BalancerConnection.cc
  src/Distributed.cc
)

target_include_directories(dst_lib
  PUBLIC
    include
)

target_link_libraries(dst_lib
  PUBLIC
    utl_lib
    ${TCL_LIBRARY}
    ${ZLIB_LIBRARIES}
    Boost::serialization
    Boost::thread
)

target_sources(dst
  PRIVATE
  src/MakeDistributed.cc
)

target_include_directories(dst
  PUBLIC
    include
)

target_link_libraries(dst
  PUBLIC
    dst_lib
)

messages(
  TARGET dst
)

add_subdirectory(test)
