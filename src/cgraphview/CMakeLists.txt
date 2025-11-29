# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025-2025, The OpenROAD Authors

include("openroad")
swig_lib(NAME         cgv
         NAMESPACE    cgv
         I_FILE       src/cgv.i
         SCRIPTS      src/cgv.tcl
         SWIG_INCLUDES ${ODB_HOME}/src/swig/common
                       ${ODB_HOME}/src/swig/tcl
                       ${ODB_HOME}/include
)

# exa_lib is intended to be the functional code of this module with
# without GUI dependencies.  C++ unit test will depend on exa_lib
# to minimize pulling the many transitive dependencies of the GUI.
add_library(cgv_lib
  src/cgv.cpp
)

target_link_libraries(cgv_lib
  PUBLIC
    odb
  PRIVATE
    utl_lib
)

target_include_directories(cgv_lib
  PUBLIC
    include
  PRIVATE
    src
)

# exa is the library that includes the GUI dependent code.  OpenROAD
# dependes on exa.
target_sources(cgv
  PRIVATE
    src/MakeCGV.cpp
    src/graphics.cpp
)

target_include_directories(cgv
  PUBLIC
    include
  PRIVATE
    src
)

target_link_libraries(cgv
  PRIVATE
    ${TCL_LIBRARY}
    cgv_lib
    gui
)

messages(
  TARGET cgv
)

################################################################
# 
# if (Python3_FOUND AND BUILD_PYTHON)
#   swig_lib(NAME          cgv_py
#            NAMESPACE     cgv
#            LANGUAGE      python
#            I_FILE        src/cgv-py.i
#            SWIG_INCLUDES ${PROJECT_SOURCE_DIR}/include/dpl
#                          ${ODB_HOME}/src/swig/common
#                          ${ODB_HOME}/src/swig/python
#            SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/exa_py.py
#   )
# 
#   target_link_libraries(exa_py
#     PUBLIC
#       exa_lib
#       gui
#       odb
#   )
# 
# endif()

#if(ENABLE_TESTS)
#  add_subdirectory(test)
#endif()
