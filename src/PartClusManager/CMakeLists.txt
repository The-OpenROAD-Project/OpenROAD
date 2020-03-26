cmake_minimum_required(VERSION 3.1)

project(PartClusManager)

set(BIN_NAME "PartClusManager")

add_subdirectory(${PARTCLUSMANAGER_HOME}/third_party/Chaco/code)

set(CMAKE_CXX_STANDARD 11)

#include(${PROJECT_SOURCE_DIR}/cmake/FindTCL.cmake)
find_package(SWIG REQUIRED)

set(PARTCLUSMANAGER_WRAP ${CMAKE_CURRENT_BINARY_DIR}/TclInterfaceWrap.cpp)
add_custom_command(OUTPUT ${PARTCLUSMANAGER_WRAP}
        COMMAND ${SWIG_EXECUTABLE} -namespace -prefix PartClusManager -tcl8 -c++ -o ${PARTCLUSMANAGER_WRAP} ${CMAKE_CURRENT_SOURCE_DIR}/src/TclInterface.i   
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/TclInterface.i
        ${CMAKE_CURRENT_SOURCE_DIR}/src/TclInterface.h
)

set(PARTCLUSMANAGER_TCL_INIT ${CMAKE_CURRENT_BINARY_DIR}/PartClusManagerTclInitVar.cc)
add_custom_command(OUTPUT ${PARTCLUSMANAGER_TCL_INIT}
        COMMAND ${OPENSTA_HOME}/etc/TclEncode.tcl ${PARTCLUSMANAGER_TCL_INIT} partclusmanager_tcl_inits ${PARTCLUSMANAGER_HOME}/src/partclusmanager.tcl
        WORKING_DIRECTORY ${PARTCLUSMANAGER_HOME}/src
        DEPENDS ${PARTCLUSMANAGER_HOME}/src/partclusmanager.tcl ${OPENSTA_HOME}/etc/TclEncode.tcl
)

file(GLOB_RECURSE SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_library(${BIN_NAME} ${SRC_FILES} ${PARTCLUSMANAGER_WRAP} ${PARTCLUSMANAGER_TCL_INIT})

include_directories(${PARTCLUSMANAGER_HOME}/src
                    PRIVATE
                    ${OPENROAD_HOME}/include
		    ${OPENDB_HOME}/include
                    ${OPENSTA_HOME}/app
		    ${TCL_INCLUDE_PATH}
                    ${PARTCLUSMANAGER_HOME}/third_party/Chaco/code
                   ) 

target_link_libraries(${BIN_NAME} INTERFACE
                      ${TCL_LIB}
                      Chaco
                     )
