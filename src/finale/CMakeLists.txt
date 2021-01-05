###########################################################################
## BSD 3-Clause License
##
## Copyright (c) 2020, OpenRoad Project
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
##   and/or other materials provided with the distribution.
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
###########################################################################

set(FINALE_WRAP ${CMAKE_CURRENT_BINARY_DIR}/finale_wrap.cc)
set(FINALE_TCL_INIT ${CMAKE_CURRENT_BINARY_DIR}/FinaleTclInitVar.cc)

add_custom_command(OUTPUT ${FINALE_WRAP}
  COMMAND ${SWIG_EXECUTABLE} -tcl8 -c++ -namespace -prefix finale -I${FINALE_HOME}/include -o ${FINALE_WRAP} ${FINALE_HOME}/src/finale.i
  WORKING_DIRECTORY ${FINALE_HOME}/src
  DEPENDS
  ${FINALE_HOME}/src/finale.i
  ${FINALE_HOME}/include/finale/Finale.h
  ${OPENROAD_HOME}/src/Exception.i
)

add_custom_command(OUTPUT ${FINALE_TCL_INIT}
  COMMAND ${OPENSTA_HOME}/etc/TclEncode.tcl ${FINALE_TCL_INIT} finale_tcl_inits ${FINALE_HOME}/src/finale.tcl
  WORKING_DIRECTORY ${FINALE_HOME}/src
  DEPENDS ${FINALE_HOME}/src/finale.tcl ${OPENSTA_HOME}/etc/TclEncode.tcl
  )

add_library(finale
  src/Finale.cpp
  src/MakeFinale.cpp
  src/DensityFill.cpp
  src/graphics.cpp

  ${FINALE_WRAP}
  ${FINALE_TCL_INIT}
)

target_include_directories(finale
  PUBLIC ${FINALE_HOME}/include
  PRIVATE
  ${FINALE_HOME}/src
  ${OPENROAD_HOME}/include
  ${OPENDB_HOME}/include
  ${OPENSTA_HOME}/include
  ${TCL_INCLUDE_PATH}
  ${Boost_INCLUDE_DIRS}
 )

target_link_libraries( finale
  PRIVATE
  opendb
  gui
)
