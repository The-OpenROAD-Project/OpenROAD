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

set(TOOL_WRAP ${CMAKE_CURRENT_BINARY_DIR}/Tool_wrap.cc)
set(TOOL_TCL_INIT ${CMAKE_CURRENT_BINARY_DIR}/ToolTclInitVar.cc)

add_custom_command(OUTPUT ${TOOL_WRAP}
  COMMAND ${SWIG_EXECUTABLE} -tcl8 -c++ -namespace -prefix tool -I${OPENROAD_HOME}/include -o ${TOOL_WRAP} ${TOOL_HOME}/src/Tool.i
  COMMAND ${OPENSTA_HOME}/etc/SwigCleanup.tcl ${CMAKE_CURRENT_BINARY_DIR}/Tool_wrap.cc
  WORKING_DIRECTORY ${TOOL_HOME}/src
  DEPENDS ${TOOL_HOME}/src/Tool.i
  )

add_custom_command(OUTPUT ${TOOL_TCL_INIT}
  COMMAND ${OPENSTA_HOME}/etc/TclEncode.tcl ${TOOL_TCL_INIT} tool_tcl_inits Tool.tcl
  WORKING_DIRECTORY ${TOOL_HOME}/src
  DEPENDS Tool.tcl ${OPENSTA_HOME}/etc/TclEncode.tcl
  )

add_library(tool
  MakeTool.cc
  Tool.cc
  ${TOOL_WRAP}
  ${TOOL_TCL_INIT}
  )

target_include_directories(tool
  PUBLIC ${TOOL_HOME}/include
  PRIVATE
  ${OPENROAD_HOME}/include
  ${OPENDB_HOME}/include
  ${OPENSTA_HOME}/app
  )
