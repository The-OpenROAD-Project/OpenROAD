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

set(ICEWALL_WRAP ${CMAKE_CURRENT_BINARY_DIR}/ICeWall_wrap.cc)
set(ICEWALL_TCL_INIT ${CMAKE_CURRENT_BINARY_DIR}/ICeWallTclInitVar.cc)

add_custom_command(OUTPUT ${ICEWALL_WRAP}
  COMMAND ${SWIG_EXECUTABLE} -tcl8 -c++ -namespace -prefix ICEWALL -I${OPENROAD_HOME}/include -o ${ICEWALL_WRAP} ${ICEWALL_HOME}/src/ICeWall.i
  COMMAND ${OPENSTA_HOME}/etc/SwigCleanup.tcl ${CMAKE_CURRENT_BINARY_DIR}/ICeWall_wrap.cc
  WORKING_DIRECTORY ${ICEWALL_HOME}/src
  DEPENDS ${ICEWALL_HOME}/src/ICeWall.i
  )

add_custom_command(OUTPUT ${ICEWALL_TCL_INIT}
  COMMAND ${OPENSTA_HOME}/etc/TclEncode.tcl ${ICEWALL_TCL_INIT} ICeWall_tcl_inits ICeWall.tcl
  WORKING_DIRECTORY ${ICEWALL_HOME}/src
  DEPENDS ICeWall.tcl ${OPENSTA_HOME}/etc/TclEncode.tcl
  )

add_library(ICeWall
  MakeICeWall.cc
  ICeWall.cc
  ${ICEWALL_WRAP}
  ${ICEWALL_TCL_INIT}
  )

target_include_directories(ICeWall
  PUBLIC ${ICEWALL_HOME}/include
  PRIVATE
  ${OPENROAD_HOME}/include
  ${OPENDB_HOME}/include
  ${OPENSTA_HOME}/app
  SYSTEM
  ${TCL_INCLUDE_PATH}
  )
