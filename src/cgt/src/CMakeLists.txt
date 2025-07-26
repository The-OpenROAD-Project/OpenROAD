## SPDX-License-Identifier: BSD-3-Clause
## Copyright (c) 2025, The OpenROAD Authors

include("openroad")

project(cgt)

swig_lib(NAME      cgt
         NAMESPACE cgt
         I_FILE    cgt.i
         SCRIPTS   cgt.tcl
)

target_sources(cgt
  PRIVATE
    ClockGating.cpp
    MakeClockGating.cpp
    NetworkBuilder.cpp
)

messages(
TARGET cgt
OUTPUT_DIR ..
)

target_include_directories(cgt
  PUBLIC
    ../include
  PRIVATE
    .
 )

target_link_libraries(cgt 
  PRIVATE
    odb
    dbSta
    OpenSTA
    cut
    dbSta_lib
    libabc
    utl_lib
 )

if (Python3_FOUND AND BUILD_PYTHON)
  swig_lib(NAME          cgt_py
           NAMESPACE     cgt
           LANGUAGE      python
           I_FILE        cgt-py.i
           SWIG_INCLUDES ${PROJECT_SOURCE_DIR}/../include
                         ${ODB_HOME}/src/swig/common
                         ${ODB_HOME}/src/swig/python
           SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/cgt_py.py
  )

  target_include_directories(cgt_py
    PUBLIC
      ../include
  )

  target_link_libraries(cgt_py
    PUBLIC
      cgt
      odb
      dbSta
      OpenSTA
      cut
      libabc
  )

endif()
