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

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(FastRoute)
set(LIB_NAME "FastRoute")

# Print info on build
if( CMAKE_BUILD_TYPE MATCHES "[Dd][Ee][Bb][Uu][Gg]" )
        # Tell other CMake files that we're doing a debug build
        set(DEBUG_BUILD 1)

        # Tell C/C++ that we're doing a debug build
        add_definitions(-DDEBUG)
endif()

add_subdirectory(third_party/fastroute)
add_subdirectory(third_party/pdrev)

set(CMAKE_CXX_STANDARD 11)
set(Boost_USE_STATIC_LIBS ON)

# Loads swig and generates the tcl wrapper file
find_package(SWIG REQUIRED)

set(FASTROUTE_WRAP ${CMAKE_CURRENT_BINARY_DIR}/TclInterfaceWrap.cpp)
set(FASTROUTE_TCL_INIT ${CMAKE_CURRENT_BINARY_DIR}/FastrouteTclInitVar.cc)

add_custom_command(OUTPUT ${FASTROUTE_WRAP}
        COMMAND ${SWIG_EXECUTABLE} -namespace -prefix FastRoute -tcl8 -c++ -o ${FASTROUTE_WRAP} ${FASTROUTE_HOME}/src/TclInterface.i
        WORKING_DIRECTORY ${FASTROUTE_HOME}/src
        DEPENDS ${FASTROUTE_HOME}/src/TclInterface.i ${FASTROUTE_HOME}/src/TclInterface.h
)

add_custom_command(OUTPUT ${FASTROUTE_TCL_INIT}
        COMMAND ${OPENSTA_HOME}/etc/TclEncode.tcl ${FASTROUTE_TCL_INIT} fastroute_tcl_inits ${FASTROUTE_HOME}/src/fastroute.tcl
        WORKING_DIRECTORY ${FASTROUTE_HOME}/src
        DEPENDS ${FASTROUTE_HOME}/src/fastroute.tcl ${OPENSTA_HOME}/etc/TclEncode.tcl
)


set(SRC_FILES
  src/Box.cpp
  src/Coordinate.cpp
  src/DBWrapper.cpp
  src/FastRouteKernel.cpp
  src/Grid.cpp
  src/MakeFastRoute.cpp
  src/Net.cpp
  src/Netlist.cpp
  src/Pin.cpp
  src/RcTreeBuilder.cpp
  src/SteinerTree.cpp
  src/TclInterface.cpp
  )

add_library(${LIB_NAME} 
        ${SRC_FILES}
        ${FASTROUTE_WRAP}
        ${FASTROUTE_TCL_INIT})

target_include_directories(${LIB_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/fastroute/include
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/pdrev/include
        ${OPENDB_HOME}/include/
        ${OPENROAD_HOME}/include
        ${DBSTA_HOME}/include
        ${OPENSTA_HOME}/include
        ${FASTROUTE_HOME}/src
	${TCL_INCLUDE_PATH}
        ${ANTENNACHECKER_HOME}/include
        ${OPENDP_HOME}/include
        )

target_link_libraries(${LIB_NAME}
        FastRoute4.1
        pdrev)
