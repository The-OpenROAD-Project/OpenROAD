# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2025, The OpenROAD Authors

# Setting CMake version properly to ensure correct policies behavior
cmake_minimum_required(VERSION 3.14...3.16)
# Setting up OpenDB project
project(odb VERSION 0.0.1
  LANGUAGES CXX)

add_subdirectory(src/db)
add_subdirectory(src/defin)
add_subdirectory(src/defout)
add_subdirectory(src/lef)
add_subdirectory(src/lefin)
add_subdirectory(src/lefout)
add_subdirectory(src/def)
add_subdirectory(src/zutil)
add_subdirectory(src/cdl)
add_subdirectory(src/3dblox)
add_subdirectory(src/gdsin)
add_subdirectory(src/gdsout)

if(ENABLE_TESTS)
  add_subdirectory(test)
endif()

add_library(odb INTERFACE)
target_link_libraries(odb
  INTERFACE
    absl::synchronization
    db
    cdl
    3dblox
    defin
    defout
    lefin
    lefout
    zutil
    utl_lib
    gdsin
    gdsout
    ${TCL_LIBRARY}
)

############################################################################
################################# SWIG #####################################
############################################################################

include(${SWIG_USE_FILE})

add_subdirectory(src/swig/common)
if (Python3_FOUND AND BUILD_PYTHON)
  add_subdirectory(src/swig/python)
endif()
add_subdirectory(src/swig/tcl)

############################################################################
############################### DOXYGEN ####################################
############################################################################
find_package(Doxygen)

if (DOXYGEN_FOUND)
  set(DOXYGEN_EXTRACT_STATIC YES)
  set(DOXYGEN_REFERENCED_BY_RELATION YES)
  set(DOXYGEN_REFERENCES_RELATION YES)

  doxygen_add_docs(docs include)
endif()

messages(
  TARGET odb
)
