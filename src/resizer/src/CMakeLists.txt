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

set(RESIZER_WRAP ${CMAKE_CURRENT_BINARY_DIR}/Resizer_wrap.cc)
set(RESIZER_TCL_INIT ${CMAKE_CURRENT_BINARY_DIR}/ResizerTclInitVar.cc)

add_custom_command(OUTPUT ${RESIZER_WRAP}
  COMMAND ${SWIG_EXECUTABLE} -tcl8 -c++ -namespace -prefix sta -I${OPENROAD_HOME}/include -o ${RESIZER_WRAP} ${RESIZER_HOME}/src/Resizer.i
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
  SteinerTree.cc
  ${RESIZER_WRAP}
  ${RESIZER_TCL_INIT}
  )

target_include_directories(resizer
  PUBLIC ${RESIZER_HOME}/include
  PRIVATE
  ${OPENROAD_HOME}/include
  ${DBSTA_HOME}/include
  ${OPENSTA_INCLUDE_DIRS}
  ${OPENDB_HOME}/include
  ${OPENROAD_HOME}/src/flute3
  ${TCL_INCLUDE_PATH}
  )
