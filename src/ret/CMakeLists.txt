# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026-2026, The OpenROAD Authors

include("openroad")

swig_lib(NAME         ret
         NAMESPACE    ret
         I_FILE       src/ret.i
         SCRIPTS      src/ret.tcl
)

# ret_lib holds the functional code without Tcl dependencies so the C++
# unit tests can link it directly.
add_library(ret_lib
  src/SequentialUnmapper.cpp
)

target_link_libraries(ret_lib
  PUBLIC
    odb
  PRIVATE
    dbSta
    OpenSTA
    utl_lib
)

target_include_directories(ret_lib
  PUBLIC
    include
)

target_sources(ret
  PRIVATE
    src/MakeRet.cpp
)

target_include_directories(ret
  PUBLIC
    include
  PRIVATE
    src
)

target_link_libraries(ret
  PRIVATE
    ${TCL_LIBRARY}
    ret_lib
)

messages(
  TARGET ret
)

if(ENABLE_TESTS)
  add_subdirectory(test)
endif()
