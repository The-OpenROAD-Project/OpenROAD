###############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2019, University of California, San Diego.
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of source code must retain the above copyright notice, this
##   list of conditions and the following disclaimer.
##
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and#or other materials provided with the distribution.
##
## * Neither the name of the copyright holder nor the names of its
##   contributors may be used to endorse or promote products derived from
##   this software without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
## LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
## CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
## SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
##
###############################################################################

cmake_minimum_required(VERSION 3.1)

project(ioPlacement)
set(BIN_NAME "ioPlacer")

# Print info on build
if( CMAKE_BUILD_TYPE MATCHES "[Dd][Ee][Bb][Uu][Gg]" )
        # for debug type builds, turn on verbose makefiles
        set(CMAKE_VERBOSE_MAKEFILE ON)

        # Tell other CMake files that we're doing a debug build
        set(DEBUG_BUILD 1)

        # Tell C/C++ that we're doing a debug build
        add_definitions(-DDEBUG)
endif()

add_subdirectory(${IOPLACER_HOME}/src/munkres)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

set(CMAKE_CXX_STANDARD 11)

find_package(ZLIB REQUIRED)

# Loads swig and generates the tcl wrapper file
find_package(SWIG REQUIRED)

set(IOPLACER_WRAP ${CMAKE_CURRENT_BINARY_DIR}/TclInterfaceWrap.cpp)
set(IOPLACER_TCL_INIT ${CMAKE_CURRENT_BINARY_DIR}/IoplacerTclInitVar.cc)

add_custom_command(OUTPUT ${IOPLACER_WRAP}
        COMMAND ${SWIG_EXECUTABLE} -namespace -prefix ioPlacer -tcl8 -c++ -o ${IOPLACER_WRAP} ${IOPLACER_HOME}/src/TclInterface.i   
        WORKING_DIRECTORY ${IOPLACER_HOME}/src
        DEPENDS ${IOPLACER_HOME}/src/ioPlacer.tcl ${OPENSTA_HOME}/etc/TclEncode.tcl
)

add_custom_command(OUTPUT ${IOPLACER_TCL_INIT}
        COMMAND ${OPENSTA_HOME}/etc/TclEncode.tcl ${IOPLACER_TCL_INIT} ioplacer_tcl_inits ${IOPLACER_HOME}/src/ioPlacer.tcl
        WORKING_DIRECTORY ${IOPLACER_HOME}/src
        DEPENDS ${IOPLACER_HOME}/src/ioPlacer.tcl ${OPENSTA_HOME}/etc/TclEncode.tcl
)

file(GLOB_RECURSE SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
set(SRC_FILES ${SRC_FILES} ${CMAKE_CURRENT_BINARY_DIR}/TclInterfaceWrap.cpp)


add_LIBRARY(${BIN_NAME} STATIC ${SRC_FILES} ${IOPLACER_WRAP} ${IOPLACER_TCL_INIT})

target_include_directories(${BIN_NAME}
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${OPENSTA_HOME}/include
  ${OPENROAD_HOME}/include
  ${TCL_INCLUDE_PATH}
  PRIVATE
  ${IOPLACER_HOME}/src/munkres/src
  )

include_directories(${opendb_home}/include/opendb) 

target_link_libraries(${BIN_NAME}
        ${TCL_LIB}
        opendb
        lefin
	Munkres
	)
                      
                      
