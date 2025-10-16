# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2025, The OpenROAD Authors

include("openroad")

swig_lib(NAME      tap
         NAMESPACE tap
         I_FILE    tapcell.i
         SCRIPTS   tapcell.tcl
         SWIG_INCLUDES ${ODB_HOME}/src/swig/common
                       ${ODB_HOME}/src/swig/tcl
                       ${ODB_HOME}/include
)

target_sources(tap
  PRIVATE
    tapcell.cpp
    MakeTapcell.cpp
)

target_include_directories(tap
  PUBLIC
    ../include
)

target_link_libraries(tap
  PRIVATE
    utl_lib
    odb
    Boost::boost
)

messages(
  TARGET tap
  OUTPUT_DIR ..
)

if (Python3_FOUND AND BUILD_PYTHON)
  swig_lib(NAME          tap_py
           NAMESPACE     tap
           LANGUAGE      python
           I_FILE        tapcell-py.i
           SWIG_INCLUDES ${PROJECT_SOURCE_DIR}/src/tap/include
           SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/tap_py.py
  )

  target_link_libraries(tap_py
    PUBLIC
      tap
      odb
  )

endif()
