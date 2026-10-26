# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

include("openroad")

swig_lib(NAME      fsv
         NAMESPACE fsv
         I_FILE    src/FieldSolver.i
         SCRIPTS   src/FieldSolver.tcl
)

target_sources(fsv
  PRIVATE
    src/AliasTable.cpp
    src/CubeGreens.cpp
    src/CubePanels.cpp
    src/Dielectric.cpp
    src/FieldSolver.cpp
    src/GaussianSurface.cpp
    src/Geometry.cpp
    src/Itf.cpp
    src/LayerModes.cpp
    src/LayeredCubeGreens.cpp
    src/LayeredSquareGreens.cpp
    src/MakeFieldSolver.cpp
    src/NetWindow.cpp
    src/Scene.cpp
    src/Solver.cpp
    src/SpatialIndex.cpp
    src/SquareGreens.cpp
    src/Stack.cpp
    src/WorkerPool.cpp
)

target_include_directories(fsv
  PUBLIC
    include
  PRIVATE
    src
)

target_link_libraries(fsv
  PRIVATE
    odb
    utl_lib
    ${TCL_LIBRARY}
)

messages(
  TARGET fsv
)

if (Python3_FOUND AND BUILD_PYTHON)
  swig_lib(NAME          fsv_py
           NAMESPACE     fsv
           LANGUAGE      python
           I_FILE        src/FieldSolver-py.i
           SWIG_INCLUDES ${ODB_HOME}/src/swig/common
                         ${ODB_HOME}/src/swig/python
           SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/fsv_py.py
  )

  target_include_directories(fsv_py
    PRIVATE
      src
  )

  target_link_libraries(fsv_py
    PUBLIC
      fsv
      odb
  )

endif()
