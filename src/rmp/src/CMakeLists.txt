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

add_library(rmp_lib
    Restructure.cpp
    annealing_strategy.cpp
    delay_optimization_strategy.cpp
    utils.cpp
    zero_slack_strategy.cpp
)

# For ABC headers
set_source_files_properties(annealing_strategy.cpp
  PROPERTIES COMPILE_FLAGS "-Wno-error=redundant-decls -Wno-error=unused-variable -Wno-error=unused-but-set-variable"
)

target_sources(rmp
  PRIVATE
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

target_include_directories(rmp_lib
  PUBLIC
    ../include
)

target_link_libraries(rmp_lib
  PUBLIC
    absl::synchronization
    odb
    dbSta_lib
    OpenSTA
    rsz_lib
    utl_lib
    cut
    ${ABC_LIBRARY}
)

target_link_libraries(rmp
  PRIVATE
    rmp_lib
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
      cut
  )

endif()
