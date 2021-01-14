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

project(ioPlacer)

add_subdirectory(${IOPLACER_HOME}/src/munkres)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

set(IOPLACER_WRAP ${CMAKE_CURRENT_BINARY_DIR}/ioplacer_wrap.cpp)
set(IOPLACER_TCL_INIT ${CMAKE_CURRENT_BINARY_DIR}/IoplacerTclInitVar.cc)

add_custom_command(OUTPUT ${IOPLACER_WRAP}
        COMMAND ${SWIG_EXECUTABLE} -tcl8 -c++ -namespace -prefix ppl -I${IOPLACER_HOME}/include -o ${IOPLACER_WRAP} ${IOPLACER_HOME}/src/ioplacer.i
        WORKING_DIRECTORY ${IOPLACER_HOME}/src
        DEPENDS
        ${IOPLACER_HOME}/src/ioplacer.i
        ${IOPLACER_HOME}/include/ioplacer/IOPlacer.h
        ${OPENROAD_HOME}/src/Exception.i
)

add_custom_command(OUTPUT ${IOPLACER_TCL_INIT}
        COMMAND ${OPENSTA_HOME}/etc/TclEncode.tcl ${IOPLACER_TCL_INIT} ioplacer_tcl_inits ${IOPLACER_HOME}/src/ioPlacer.tcl
        WORKING_DIRECTORY ${IOPLACER_HOME}/src
        DEPENDS ${IOPLACER_HOME}/src/ioPlacer.tcl ${OPENSTA_HOME}/etc/TclEncode.tcl
)

set(SRC_FILES
  src/Core.cpp
  src/HungarianMatching.cpp
  src/IOPlacer.cpp
  src/MakeIoplacer.cpp
  src/Netlist.cpp
  ${IOPLACER_WRAP}
  ${IOPLACER_TCL_INIT}
  )


add_LIBRARY(ioPlacer STATIC ${SRC_FILES} ${IOPLACER_WRAP} ${IOPLACER_TCL_INIT})

target_include_directories(ioPlacer
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include/
  PRIVATE
  ${OPENDB_HOME}/include/
  ${OPENSTA_HOME}/include
  ${OPENROAD_HOME}/include
  ${TCL_INCLUDE_PATH}
  ${IOPLACER_HOME}/src
  ${IOPLACER_HOME}/src/munkres/src
  )

target_link_libraries(ioPlacer
  PUBLIC
	Munkres
  spdlog::spdlog
	)
                      
                      
