# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021-2025, The OpenROAD Authors

include("openroad")

find_package(Eigen3 REQUIRED)

swig_lib(NAME      psm
         NAMESPACE psm
         I_FILE    pdnsim.i
         SWIG_INCLUDES ${OPENSTA_HOME}
         SCRIPTS   pdnsim.tcl
)

target_sources(psm
  PRIVATE
    MakePDNSim.cpp
    pdnsim.cpp
    heatMap.cpp
    ir_solver.cpp
    ir_network.cpp
    connection.cpp
    shape.cpp
    node.cpp

    debug_gui.cpp
)

target_include_directories( psm
  PUBLIC
    ../include
  PRIVATE
    # A side-effect of including OpenSTA swig files
    ${OPENSTA_HOME}
    ${OPENSTA_HOME}/include/sta
)

target_link_libraries(psm
  PRIVATE
    utl_lib
    odb
    OpenSTA
    dbSta
    dpl_lib
    est_lib
    Eigen3::Eigen
    gui
    Boost::boost
)

messages(
  TARGET psm
  OUTPUT_DIR ..
)

if (Python3_FOUND AND BUILD_PYTHON)
  swig_lib(NAME          psm_py
           NAMESPACE     psm
           LANGUAGE      python
           I_FILE        pdnsim-py.i
           SWIG_INCLUDES ${PROJECT_SOURCE_DIR}/../include
                         ${ODB_HOME}/src/swig/common
                         ${ODB_HOME}/src/swig/python
                         ${ODB_HOME}/include
           SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/psm_py.py
  )

  target_include_directories(psm_py
    PUBLIC
      ../include
  )

  target_link_libraries(psm_py
    PUBLIC
      odb
      psm
  )

endif()
