# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors

include("openroad")
swig_lib(NAME         exa
         NAMESPACE    exa
         I_FILE       src/example.i
         SCRIPTS      src/example.tcl
         SWIG_INCLUDES ${ODB_HOME}/src/swig/common
                       ${ODB_HOME}/src/swig/tcl
                       ${ODB_HOME}/include
)

# exa_lib is intended to be the functional code of this module with
# without GUI dependencies.  C++ unit test will depend on exa_lib
# to minimize pulling the many transitive dependencies of the GUI.
add_library(exa_lib
  src/example.cpp
)

target_link_libraries(exa_lib
  PUBLIC
    odb
  PRIVATE
    utl_lib
)

target_include_directories(exa_lib
  PUBLIC
    include
  PRIVATE
    src
)

# exa is the library that includes the GUI dependent code.  OpenROAD
# dependes on exa.
target_sources(exa
  PRIVATE
    src/MakeExample.cpp
    src/graphics.cpp
)

target_include_directories(exa
  PUBLIC
    include
  PRIVATE
    src
)

target_link_libraries(exa
  PRIVATE
    ${TCL_LIBRARY}
    exa_lib
    gui
)

messages(
  TARGET exa
)

################################################################

if (Python3_FOUND AND BUILD_PYTHON)
  swig_lib(NAME          exa_py
           NAMESPACE     exa
           LANGUAGE      python
           I_FILE        src/example-py.i
           SWIG_INCLUDES ${PROJECT_SOURCE_DIR}/include/dpl
                         ${ODB_HOME}/src/swig/common
                         ${ODB_HOME}/src/swig/python
           SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/exa_py.py
  )

  target_link_libraries(exa_py
    PUBLIC
      exa_lib
      gui
      odb
  )

endif()

if(ENABLE_TESTS)
  add_subdirectory(test)
endif()
