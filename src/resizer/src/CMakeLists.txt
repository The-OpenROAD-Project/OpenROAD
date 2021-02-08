############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2019, James Cherry, Parallax Software, Inc.
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
##
############################################################################

set(RESIZER_WRAP ${CMAKE_CURRENT_BINARY_DIR}/Resizer_wrap.cc)
set(RESIZER_TCL_INIT ${CMAKE_CURRENT_BINARY_DIR}/ResizerTclInitVar.cc)

add_custom_command(OUTPUT ${RESIZER_WRAP}
  COMMAND ${SWIG_EXECUTABLE} -tcl8 -c++ -namespace -prefix rsz -I${OPENROAD_HOME}/include -o ${RESIZER_WRAP} ${RESIZER_HOME}/src/Resizer.i
  COMMAND ${OPENSTA_HOME}/etc/SwigCleanup.tcl ${RESIZER_WRAP}
  WORKING_DIRECTORY ${RESIZER_HOME}/src
  DEPENDS
  ${RESIZER_HOME}/src/Resizer.i
  ${RESIZER_HOME}/include/resizer/Resizer.hh
  )

add_custom_command(OUTPUT ${RESIZER_TCL_INIT}
  COMMAND ${OPENSTA_HOME}/etc/TclEncode.tcl ${RESIZER_TCL_INIT} resizer_tcl_inits Resizer.tcl
  WORKING_DIRECTORY ${RESIZER_HOME}/src
  DEPENDS Resizer.tcl ${OPENSTA_HOME}/etc/TclEncode.tcl
  )

add_library(resizer
  MakeResizer.cc
  Resizer.cc
  Rebuffer.cc
  SteinerTree.cc
  ${RESIZER_WRAP}
  ${RESIZER_TCL_INIT}
  )

target_include_directories(resizer
  PUBLIC ${RESIZER_HOME}/include
  PRIVATE
  ${OPENROAD_HOME}/include
  ${DBSTA_HOME}/include
  ${OPENSTA_HOME}/include
  ${OPENDB_HOME}/include
  ${OPENROAD_HOME}/src/flute3
  ${GUI_HOME}/include
  ${TCL_INCLUDE_PATH}
  )

target_link_libraries(resizer
  PUBLIC
  utility
  )
