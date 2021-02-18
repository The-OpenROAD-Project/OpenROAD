############################################################################
########################### Project Setup ##################################
############################################################################

# Setting CMake version properly to ensure correct policies behavior
cmake_minimum_required(VERSION 3.14...3.16)
# Setting up OpenDB project
project(OpenDB VERSION 0.0.1
  LANGUAGES CXX)

# Configure build version
configure_file (
    "${PROJECT_SOURCE_DIR}/include/opendb/version.hh.in"
    "${PROJECT_BINARY_DIR}/include/opendb/version.hh"
)

############################################################################
################################# Targets ##################################
############################################################################

add_subdirectory(src/db)
add_subdirectory(src/defin)
add_subdirectory(src/defout)
add_subdirectory(src/lef)
add_subdirectory(src/lefin)
add_subdirectory(src/lefout)
add_subdirectory(src/def)
add_subdirectory(src/zutil)
add_subdirectory(src/zlib)
add_subdirectory(src/tm)
add_subdirectory(src/cdl)
add_subdirectory(tests/cpp)
############################################################################
################################# SWIG #####################################
############################################################################

include(${SWIG_USE_FILE})

option(BUILD_PYTHON "Build Python SWIG module" ON)
if(BUILD_PYTHON)
    add_subdirectory(src/swig/python)
endif()

option(BUILD_TCL "Build TCL SWIG module" ON)
if(BUILD_TCL)
    add_subdirectory(src/swig/tcl)
endif()

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
