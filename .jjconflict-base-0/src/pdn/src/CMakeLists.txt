# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2025, The OpenROAD Authors

include("openroad")

swig_lib(NAME      pdn
         NAMESPACE pdn
         I_FILE    PdnGen.i
         SWIG_INCLUDES ${ODB_HOME}/src/swig/common
                       ${ODB_HOME}/src/swig/tcl
                       ${ODB_HOME}/include
         SCRIPTS   pdn.tcl
)

target_sources(pdn
  PRIVATE
    PdnGen.cc
    MakePdnGen.cc
    
    techlayer.cpp
    domain.cpp
    power_cells.cpp
    grid.cpp
    grid_component.cpp
    rings.cpp
    straps.cpp
    shape.cpp
    via.cpp
    connect.cpp
    renderer.cpp
    sroute.cpp
    via_repair.cpp
)

target_include_directories(pdn
  PUBLIC
    ../include
)

target_link_libraries(pdn
  PRIVATE
    odb
    utl_lib
    gui
    Boost::boost
)

messages(
  TARGET pdn
  OUTPUT_DIR ..
)

if (Python3_FOUND AND BUILD_PYTHON)
  swig_lib(NAME          pdn_py
           NAMESPACE     pdn
           LANGUAGE      python
           I_FILE        PdnGen-py.i
           SWIG_INCLUDES ${ODB_HOME}/src/swig/common
                         ${ODB_HOME}/src/swig/python
                         ${ODB_HOME}/include
                         ${PROJECT_SOURCE_DIR}/../include/pdn
           SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/pdn_py.py
  )

  target_include_directories(pdn_py
    PUBLIC
      .
  )
  
  target_link_libraries(pdn_py
    PUBLIC
      pdn
      odb
      gui
    )

endif()
