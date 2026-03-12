# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2020-2025, The OpenROAD Authors

include("openroad")

find_package(OpenMP REQUIRED)

swig_lib(NAME      ant
         NAMESPACE ant
         I_FILE    AntennaChecker.i
         SCRIPTS   AntennaChecker.tcl
)

add_library(ant_lib
    AntennaChecker.cc
    Polygon.cc
    WireBuilder.cc
)

target_sources(ant
  PRIVATE
    MakeAntennaChecker.cc
)

target_include_directories(ant_lib
  PUBLIC
    ../include
)

target_link_libraries(ant_lib
  PUBLIC
    odb
    utl_lib
    OpenMP::OpenMP_CXX
)

target_link_libraries(ant
  PUBLIC
    absl::synchronization
    ant_lib
    grt_lib
    utl_lib
    OpenMP::OpenMP_CXX
)

messages(
  TARGET ant
  OUTPUT_DIR ..
)

if (Python3_FOUND AND BUILD_PYTHON)
  swig_lib(NAME          ant_py
           NAMESPACE     ant
           LANGUAGE      python
           I_FILE        AntennaChecker-py.i
           SWIG_INCLUDES ${PROJECT_SOURCE_DIR}/../include
                         ${ODB_HOME}/src/swig/common
                         ${ODB_HOME}/src/swig/python
           SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/ant_py.py
  )

  target_include_directories(ant_py
    PUBLIC
      ../include
  )

  target_link_libraries(ant_py
    PUBLIC
      odb
      grt
      utl_lib
  )

endif()
