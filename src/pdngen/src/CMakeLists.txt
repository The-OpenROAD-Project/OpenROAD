#BSD 3-Clause License
#
#Copyright (c) 2019, The Regents of the University of California
#All rights reserved.
#
#Redistribution and use in source and binary forms, with or without
#modification, are permitted provided that the following conditions are met:
#
#1. Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
#2. Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
#3. Neither the name of the copyright holder nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
#
#THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
#FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
#SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
#CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
#OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

set(PDNGEN_WRAP ${CMAKE_CURRENT_BINARY_DIR}/PdnGen_wrap.cc)
set(PDNGEN_TCL_INIT ${CMAKE_CURRENT_BINARY_DIR}/PdnGenTclInitVar.cc)

add_custom_command(OUTPUT ${PDNGEN_WRAP}
  COMMAND ${SWIG_EXECUTABLE} -tcl8 -c++ -namespace -prefix pdngen -I${OPENROAD_HOME}/include -o ${PDNGEN_WRAP} ${PDNGEN_HOME}/src/PdnGen.i
  COMMAND ${OPENSTA_HOME}/etc/SwigCleanup.tcl ${CMAKE_CURRENT_BINARY_DIR}/PdnGen_wrap.cc
  WORKING_DIRECTORY ${PDNGEN_HOME}/src
  DEPENDS ${PDNGEN_HOME}/src/PdnGen.i
  )

add_custom_command(OUTPUT ${PDNGEN_TCL_INIT}
  COMMAND ${OPENSTA_HOME}/etc/TclEncode.tcl ${PDNGEN_TCL_INIT} pdngen_tcl_inits PdnGen.tcl
  WORKING_DIRECTORY ${PDNGEN_HOME}/src
  DEPENDS PdnGen.tcl ${OPENSTA_HOME}/etc/TclEncode.tcl
  )

add_library(pdngen
  MakePdnGen.cc
  PdnGen.cc
  ${PDNGEN_WRAP}
  ${PDNGEN_TCL_INIT}
  )

target_include_directories(pdngen
  PUBLIC ${PDNGEN_HOME}/include
  PRIVATE
  ${OPENROAD_HOME}/include
  ${OPENDB_HOME}/include
  ${OPENSTA_HOME}/app
  )
