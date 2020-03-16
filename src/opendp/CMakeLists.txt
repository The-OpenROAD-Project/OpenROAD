cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(OpenDP)

find_package(SWIG REQUIRED)

set(OPENDP_WRAP ${CMAKE_CURRENT_BINARY_DIR}/opendp_wrap.cc)
set(OPENDP_TCL_INIT ${CMAKE_CURRENT_BINARY_DIR}/OpendpTclInitVar.cc)

add_custom_command(OUTPUT ${OPENDP_WRAP}
  COMMAND ${SWIG_EXECUTABLE} -tcl8 -c++ -namespace -prefix opendp -I${OPENDP_HOME}/include -o ${OPENDP_WRAP} ${OPENDP_HOME}/src/opendp.i
  WORKING_DIRECTORY ${OPENDP_HOME}/src
  DEPENDS
  ${OPENDP_HOME}/src/opendp.i
  ${OPENDP_HOME}/include/opendp/Opendp.h
  ${OPENROAD_HOME}/src/Exception.i
)

add_custom_command(OUTPUT ${OPENDP_TCL_INIT}
  COMMAND ${OPENSTA_HOME}/etc/TclEncode.tcl ${OPENDP_TCL_INIT} opendp_tcl_inits ${OPENDP_HOME}/src/opendp.tcl
  WORKING_DIRECTORY ${OPENDP_HOME}/src
  DEPENDS ${OPENDP_HOME}/src/opendp.tcl ${OPENSTA_HOME}/etc/TclEncode.tcl
  )

############################################################
# Libraries setting 
############################################################

add_library(opendp
  src/Opendp.cpp
  src/dbToOpendp.cpp
  src/MakeOpendp.cpp
  src/assign.cpp
  src/CheckPlacement.cpp
  src/place.cpp
  src/utility.cpp
  src/FillerPlacement.cpp

  ${OPENDP_WRAP}
  ${OPENDP_TCL_INIT}
)

target_include_directories(opendp
  PUBLIC ${OPENDP_HOME}/include
  PRIVATE
  ${OPENDP_HOME}/src
  ${OPENROAD_HOME}/include
  ${OPENDB_HOME}/include
  ${OPENSTA_HOME}/app
  ${TCL_INCLUDE_PATH}
 )
