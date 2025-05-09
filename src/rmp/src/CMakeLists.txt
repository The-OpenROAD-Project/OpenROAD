# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2025, The OpenROAD Authors

include("openroad")

project(rmp)

swig_lib(NAME      rmp
         NAMESPACE rmp
         I_FILE    rmp.i
         SCRIPTS   rmp.tcl
         SWIG_INCLUDES ${OPENSTA_HOME}
)

target_sources(rmp
  PRIVATE
    blifParser.cpp
    blif.cpp
    Restructure.cpp
    MakeRestructure.cpp
)

messages(
TARGET rmp
OUTPUT_DIR ..
)

target_include_directories(rmp
  PUBLIC
    ../include
  PRIVATE
    .
    # Unfortunate side-effect of swig include StaTcl.i
    ${OPENSTA_HOME}/include/sta
    ${OPENSTA_HOME}
 )

target_link_libraries(rmp 
  PRIVATE
    odb
    dbSta
    OpenSTA
    rsz
    utl_lib
    rmp_abc_library
    ${ABC_LIBRARY}
 )

add_library(rmp_abc_library 
  abc_library_factory.cpp
  logic_extractor.cpp
  logic_cut.cpp
  delay_optimization_strategy.cpp
  zero_slack_strategy.cpp
)

target_include_directories(rmp_abc_library
  PUBLIC
    ../include
  PRIVATE
    .
)

target_link_libraries(rmp_abc_library
  PUBLIC
    OpenSTA
    dbSta_lib
    libabc
    utl_lib
    ${ABC_LIBRARY}
)

if (Python3_FOUND AND BUILD_PYTHON)
  swig_lib(NAME          rmp_py
           NAMESPACE     rmp
           LANGUAGE      python
           I_FILE        rmp-py.i
           SWIG_INCLUDES ${OPENSTA_HOME}
                         ${PROJECT_SOURCE_DIR}/../include
                         ${ODB_HOME}/src/swig/common
                         ${ODB_HOME}/src/swig/python
           SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/rmp_py.py
  )

  target_include_directories(rmp_py
    PUBLIC
      ../include
  )

  target_link_libraries(rmp_py
    PUBLIC
      rmp
      odb
      dbSta
      OpenSTA
      rsz
  )

endif()
