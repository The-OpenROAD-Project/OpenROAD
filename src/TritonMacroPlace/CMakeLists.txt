cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(THREADS_PREFER_PTHREAD_FLAG ON)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(ABKCOMMON_HOME ${MPLACE_HOME}/module/ABKCommon )
set(PARQUET_HOME ${MPLACE_HOME}/module/ParquetFP )

add_subdirectory(${ABKCOMMON_HOME})
add_subdirectory(${PARQUET_HOME})

set(ENV{TYPE_CALL} library)


## configure settings
find_package(TCL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(SWIG REQUIRED)
find_package(Eigen3 REQUIRED)

# for cimg
find_package(Threads REQUIRED)


############################################################
# Create a library 
############################################################

set(MPLACE_WRAP ${CMAKE_CURRENT_BINARY_DIR}/TritonMp_wrap.cc)
set(MPLACE_TCL_INIT ${CMAKE_CURRENT_BINARY_DIR}/TritonMpTclInitVar.cc)


add_custom_command(OUTPUT ${MPLACE_WRAP}
  COMMAND ${SWIG_EXECUTABLE} -tcl8 -c++ -o ${MPLACE_WRAP} -I${MPLACE_HOME}/include ${MPLACE_HOME}/src/macroPlace.i   
  WORKING_DIRECTORY ${MPLACE_HOME}
  DEPENDS ${MPLACE_HOME}/src/macroPlace.i "src/TritonMacroPlace.cpp" "include/tritonmp/TritonMacroPlace.h"
)

add_custom_command(OUTPUT ${MPLACE_TCL_INIT}
  COMMAND ${OPENSTA_HOME}/etc/TclEncode.tcl ${MPLACE_TCL_INIT} tritonmp_tcl_inits ${MPLACE_HOME}/src/macroPlace.tcl
  WORKING_DIRECTORY ${MPLACE_HOME}/src
  DEPENDS ${MPLACE_HOME}/src/macroPlace.tcl ${OPENSTA_HOME}/etc/TclEncode.tcl
)

############################################################
# Create an library 
############################################################
add_library( TritonMacroPlace 
  src/circuit.cpp
  src/partition.cpp
  src/graph.cpp
  src/macro.cpp
  src/TritonMacroPlace.cpp
  src/macroPlace.cpp
  src/MakeTritonMp.cpp
  src/hashUtil.cpp
  src/logger.cpp
  ${MPLACE_WRAP}
  ${MPLACE_TCL_INIT}
)

############################################################
# include directories 
############################################################

target_include_directories( TritonMacroPlace 
PUBLIC
  ${MPLACE_HOME}/src
  ${MPLACE_HOME}/include

PRIVATE
  ${ABKCOMMON_HOME}
  ${PARQUET_HOME}/src

  ${OPENSTA_HOME}/include

  ${OPENDB_HOME}/include
  ${OPENROAD_HOME}/include
  ${DBSTA_HOME}/include

  ${TCL_INCLUDE_PATH}
  )

target_link_libraries( TritonMacroPlace
  Eigen3::Eigen
)
