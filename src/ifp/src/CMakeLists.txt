# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2025, The OpenROAD Authors

include("openroad")

swig_lib(NAME      ifp
         NAMESPACE ifp
         I_FILE    InitFloorplan.i
         SCRIPTS   InitFloorplan.tcl
         SWIG_INCLUDES ${ODB_HOME}/src/swig/common
                       ${ODB_HOME}/src/swig/tcl
                       ${ODB_HOME}/include
)

target_sources(ifp
  PRIVATE
    InitFloorplan.cc
    MakeInitFloorplan.cc
)

target_include_directories(ifp
  PUBLIC
    ../include
)

target_link_libraries(ifp
  PRIVATE
    odb
    dbSta
    OpenSTA
    utl_lib
    upf_lib
)

if (Python3_FOUND AND BUILD_PYTHON)
  swig_lib(NAME          ifp_py
           NAMESPACE     ifp
           LANGUAGE      python
           I_FILE        InitFloorplan-py.i
           SWIG_INCLUDES ${ODB_HOME}/src/swig/common
                         ${ODB_HOME}/src/swig/python
           SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/ifp_py.py
  )

  target_link_libraries(ifp_py
    PUBLIC
      ifp
      odb
  )
endif()

messages(
  TARGET ifp
  OUTPUT_DIR ..
)
