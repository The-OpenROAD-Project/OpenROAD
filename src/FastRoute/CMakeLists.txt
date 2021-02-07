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

project(FastRoute)

add_subdirectory(src/fastroute)
add_subdirectory(src/pdrev)

set(FASTROUTE_WRAP ${CMAKE_CURRENT_BINARY_DIR}/fastroute_wrap.cpp)
set(FASTROUTE_TCL_INIT ${CMAKE_CURRENT_BINARY_DIR}/FastrouteTclInitVar.cc)

add_custom_command(OUTPUT ${FASTROUTE_WRAP}
        COMMAND ${SWIG_EXECUTABLE} -tcl8 -c++ -namespace -prefix grt -I${FASTROUTE_HOME}/include -o ${FASTROUTE_WRAP} ${FASTROUTE_HOME}/src/fastroute.i
        WORKING_DIRECTORY ${FASTROUTE_HOME}/src
        DEPENDS
        ${FASTROUTE_HOME}/src/fastroute.i
        ${FASTROUTE_HOME}/include/fastroute/GlobalRouter.h
        ${OPENROAD_HOME}/src/Exception.i
)

add_custom_command(OUTPUT ${FASTROUTE_TCL_INIT}
        COMMAND ${OPENSTA_HOME}/etc/TclEncode.tcl ${FASTROUTE_TCL_INIT} fastroute_tcl_inits ${FASTROUTE_HOME}/src/fastroute.tcl
        WORKING_DIRECTORY ${FASTROUTE_HOME}/src
        DEPENDS ${FASTROUTE_HOME}/src/fastroute.tcl ${OPENSTA_HOME}/etc/TclEncode.tcl
)


set(SRC_FILES
  src/AntennaRepair.cpp
  src/GlobalRouter.cpp
  src/Grid.cpp
  src/MakeFastRoute.cpp
  src/Net.cpp
  src/Pin.cpp
  src/RcTreeBuilder.cpp
  ${FASTROUTE_WRAP}
  ${FASTROUTE_TCL_INIT}
  )

add_library(FastRoute ${SRC_FILES})

target_include_directories(FastRoute
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/fastroute/include
  ${CMAKE_CURRENT_SOURCE_DIR}/pdrev/include
  ${OPENDB_HOME}/include/
  ${OPENROAD_HOME}/include
  ${DBSTA_HOME}/include
  ${OPENSTA_HOME}/include
  ${FASTROUTE_HOME}/src
  ${GUI_HOME}/include
  ${TCL_INCLUDE_PATH}
  ${ANTENNACHECKER_HOME}/include
  ${OPENDP_HOME}/include
  ${Boost_INCLUDE_DIRS}
  )

target_link_libraries(FastRoute
  PUBLIC
  utility
  PRIVATE
  FastRoute4.1
  pdrev
  )
