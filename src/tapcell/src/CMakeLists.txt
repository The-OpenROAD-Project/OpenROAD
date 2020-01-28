################################################################################
## Authors: Eder Matheus Monteiro (UFRGS) and Minsoo Kim (UCSD)
##
## BSD 3-Clause License
##
## Copyright (c) 2019, Federal University of Rio Grande do Sul (UFRGS)
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
################################################################################

set(TAPCELL_WRAP ${CMAKE_CURRENT_BINARY_DIR}/tapcell_wrap.cpp)
set(TAPCELL_TCL_INIT ${CMAKE_CURRENT_BINARY_DIR}/TapcellTclInitVar.cpp)

add_custom_command(OUTPUT ${TAPCELL_WRAP}
  COMMAND ${SWIG_EXECUTABLE} -tcl8 -c++ -namespace -prefix Tapcell -I${OPENROAD_HOME}/include -o ${TAPCELL_WRAP} ${TAPCELL_HOME}/src/tapcell.i
  COMMAND ${OPENSTA_HOME}/etc/SwigCleanup.tcl ${TAPCELL_WRAP}
  WORKING_DIRECTORY ${TAPCELL_HOME}/src
  DEPENDS ${TAPCELL_HOME}/src/tapcell.i
  )

add_custom_command(OUTPUT ${TAPCELL_TCL_INIT}
  COMMAND ${OPENSTA_HOME}/etc/TclEncode.tcl ${TAPCELL_TCL_INIT} tapcell_tcl_inits tapcell.tcl
  WORKING_DIRECTORY ${TAPCELL_HOME}/src
  DEPENDS tapcell.tcl ${OPENSTA_HOME}/etc/TclEncode.tcl
  )

add_library(tapcell
  tapcell.cpp
  MakeTapcell.cpp
  ${TAPCELL_WRAP}
  ${TAPCELL_TCL_INIT}
  )

target_include_directories(tapcell
  PUBLIC ${TAPCELL_HOME}/include
  PRIVATE
  ${OPENROAD_HOME}/include
  ${OPENDB_HOME}/include
  ${OPENSTA_HOME}/app
  ${TCL_INCLUDE_PATH}
  )
