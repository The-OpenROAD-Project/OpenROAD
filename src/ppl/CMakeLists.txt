# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2025, The OpenROAD Authors

include("openroad")

project(ppl)

add_subdirectory(src/munkres)

swig_lib(NAME      ppl
         NAMESPACE ppl
         I_FILE    src/IOPlacer.i
         SCRIPTS   src/IOPlacer.tcl
)

target_sources(ppl
  PRIVATE
    src/Core.cpp
    src/HungarianMatching.cpp
    src/IOPlacer.cpp
    src/IOPlacerRenderer.cpp
    src/MakeIoplacer.cpp
    src/Netlist.cpp
    src/SimulatedAnnealing.cpp
    src/Slots.cpp
)


target_include_directories(ppl
  PUBLIC
    include
  PRIVATE
    src
)

target_link_libraries(ppl
  PUBLIC
    Munkres
    odb
    utl_lib
    gui
    Boost::boost
)
                      
messages(
  TARGET ppl
)

if (Python3_FOUND AND BUILD_PYTHON)
  swig_lib(NAME          ppl_py
           NAMESPACE     ppl
           LANGUAGE      python
           I_FILE        src/IOPlacer-py.i
           SWIG_INCLUDES ${PROJECT_SOURCE_DIR}/include/ppl
                         ${ODB_HOME}/src/swig/common
                         ${ODB_HOME}/src/swig/python
           SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/ppl_py.py
  )

  target_link_libraries(ppl_py
    PUBLIC
      ppl
      odb
      Boost::boost
  )

endif()

add_subdirectory(test)
