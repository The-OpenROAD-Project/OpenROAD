# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2020-2025, The OpenROAD Authors

include("openroad")

swig_lib(NAME      fin
         NAMESPACE fin
         I_FILE    src/finale.i
         SCRIPTS   src/finale.tcl
)

target_sources(fin
  PRIVATE
    src/Finale.cpp
    src/MakeFinale.cpp
    src/DensityFill.cpp
    src/graphics.cpp
)

target_include_directories(fin
  PUBLIC
    include
)

target_link_libraries(fin
  PRIVATE
    odb
    gui
    Boost::boost
)

messages(
  TARGET fin
)

if (Python3_FOUND AND BUILD_PYTHON)
  swig_lib(NAME          fin_py
           NAMESPACE     fin
           LANGUAGE      python
           I_FILE        src/finale-py.i
           SWIG_INCLUDES ${PROJECT_SOURCE_DIR}/include/fin
           SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/fin_py.py
  )

  target_link_libraries(fin_py
    PUBLIC
      fin
      odb
  )

endif()

add_subdirectory(test)
