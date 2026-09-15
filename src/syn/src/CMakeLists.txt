# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

include("openroad")

add_subdirectory(ir)
add_subdirectory(flow)
add_subdirectory(elab)

add_library(syn_lib
  synthesis.cpp
)

target_include_directories(syn_lib
  PUBLIC
    ../include
)

target_link_libraries(syn_lib
  PUBLIC
    syn_elab
    syn_flow
    syn_ir
    dbSta_lib
    odb
    utl_lib
    OpenSTA
)

swig_lib(NAME      syn
         NAMESPACE syn
         I_FILE    synthesis.i
         SWIG_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}
         SCRIPTS   synthesis.tcl
)

target_sources(syn
  PRIVATE
    MakeSynthesis.cpp
)

target_include_directories(syn
  PUBLIC
    ../include
)

target_link_libraries(syn
  PUBLIC
    syn_lib
)

messages(
  TARGET syn
  OUTPUT_DIR ..
)

if (Python3_FOUND AND BUILD_PYTHON)
  swig_lib(NAME          syn_py
           NAMESPACE     syn
           LANGUAGE      python
           I_FILE        synthesis-py.i
           SWIG_INCLUDES ${PROJECT_SOURCE_DIR}
                         ${ODB_HOME}/src/swig/common
                         ${ODB_HOME}/src/swig/python
           SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/syn_py.py
  )

  target_include_directories(syn_py
    PUBLIC
      ../include
  )

  target_link_libraries(syn_py
    PUBLIC
      syn_lib
      odb
  )
endif()
