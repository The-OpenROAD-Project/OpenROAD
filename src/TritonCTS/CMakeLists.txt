cmake_minimum_required(VERSION 3.1)

project(TritonCTS)

set(BIN_NAME "TritonCTS")

set(CMAKE_CXX_STANDARD 11)

#include(${PROJECT_SOURCE_DIR}/cmake/FindTCL.cmake)
find_package(SWIG REQUIRED)
find_package(LEMON REQUIRED)

set(TRITONCTS_WRAP ${CMAKE_CURRENT_BINARY_DIR}/TritonCTSKernelTclWrap.cpp)
add_custom_command(OUTPUT ${TRITONCTS_WRAP}
        COMMAND ${SWIG_EXECUTABLE} -tcl8 -c++ -o ${TRITONCTS_WRAP} ${CMAKE_CURRENT_SOURCE_DIR}/src/TritonCTSKernelTcl.i   
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/TritonCTSKernelTcl.i
        ${CMAKE_CURRENT_SOURCE_DIR}/src/TritonCTSKernel.h
        ${OPENROAD_HOME}/src/Exception.i
)

set(TRITONCTS_TCL_INIT ${CMAKE_CURRENT_BINARY_DIR}/TritonCtsTclInitVar.cc)
add_custom_command(OUTPUT ${TRITONCTS_TCL_INIT}
        COMMAND ${OPENSTA_HOME}/etc/TclEncode.tcl ${TRITONCTS_TCL_INIT} tritoncts_tcl_inits ${TRITONCTS_HOME}/src/tritoncts.tcl
        WORKING_DIRECTORY ${TRITONCTS_HOME}/src
        DEPENDS ${TRITONCTS_HOME}/src/tritoncts.tcl ${OPENSTA_HOME}/etc/TclEncode.tcl
)

file(GLOB_RECURSE SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_library(${BIN_NAME} ${SRC_FILES} ${TRITONCTS_WRAP} ${TRITONCTS_TCL_INIT})


include_directories(${TRITONCTS_HOME}/src
                    PRIVATE
                    ${OPENDB_HOME}/include
                    ${OPENDB_HOME}/include/opendb
                    ${OPENROAD_HOME}/include
                    ${DBSTA_HOME}/include
                    ${OPENSTA_HOME}/include
		    ${TCL_INCLUDE_PATH}
                    ${LEMON_INCLUDE_DIRS}
                   ) 

target_link_libraries(${BIN_NAME} INTERFACE
                      ${TCL_LIB}
                      lefin
                      opendb
                     )
