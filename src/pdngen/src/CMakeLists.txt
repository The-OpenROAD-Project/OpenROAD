# Copyright (c) 2019, Parallax Software, Inc.
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

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
