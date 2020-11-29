cmake_minimum_required(VERSION 3.9)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(PDNSIM_WRAP ${CMAKE_CURRENT_BINARY_DIR}/pdnsim_wrap.cpp)
set(PDNSIM_TCL_INIT ${CMAKE_CURRENT_BINARY_DIR}/PDNSimTclInitVar.cc)

## configure settings
find_package(Eigen3 REQUIRED)

set (PDNSIM_SRC 
  node.cpp
  gmat.cpp
  ir_solver.cpp
  get_power.cpp
  get_voltage.cpp
  pdnsim.cpp
  MakePDNSim.cpp
  ${PDNSIM_WRAP}
  ${PDNSIM_TCL_INIT}
  )

add_custom_command(OUTPUT ${PDNSIM_WRAP}
  COMMAND ${SWIG_EXECUTABLE} -tcl8 -c++ -o ${PDNSIM_WRAP} ${PDNSIM_HOME}/src/pdnsim.i 
  WORKING_DIRECTORY ${PDNSIM_HOME}
  DEPENDS ${PDNSIM_HOME}/src/pdnsim.i ${PDNSIM_HOME}/include/pdnsim/pdnsim.h
)

add_custom_command(OUTPUT ${PDNSIM_TCL_INIT}
  COMMAND ${OPENSTA_HOME}/etc/TclEncode.tcl ${PDNSIM_TCL_INIT} pdnsim_tcl_inits ${PDNSIM_HOME}/src/pdnsim.tcl
  WORKING_DIRECTORY ${PDNSIM_HOME}/src
  DEPENDS ${PDNSIM_HOME}/src/pdnsim.tcl ${OPENSTA_HOME}/etc/TclEncode.tcl
)

############################################################
# Create a library 
############################################################
add_library( pdnsim 
    ${PDNSIM_SRC}
)


############################################################
# include directories 
############################################################

target_include_directories( pdnsim
PUBLIC
  ${PDNSIM_HOME}/include
PRIVATE
  ${OPENROAD_HOME}/include
  ${DBSTA_HOME}/include

  ${PDNSIM_HOME}/src

  ${OPENSTA_HOME}/include

  ${OPENDB_HOME}/include
  ${TCL_INCLUDE_PATH}
)

target_link_libraries(pdnsim
  Eigen3::Eigen
)
