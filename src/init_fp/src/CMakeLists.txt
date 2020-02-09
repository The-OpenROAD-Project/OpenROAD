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

set(INIT_FP_WRAP ${CMAKE_CURRENT_BINARY_DIR}/InitFloorplan_wrap.cc)
set(INIT_FP_TCL_INIT ${CMAKE_CURRENT_BINARY_DIR}/InitFloorplanTclInitVar.cc)

add_custom_command(OUTPUT ${INIT_FP_WRAP}
  COMMAND ${SWIG_EXECUTABLE} -tcl8 -c++ -namespace -prefix ord -I${INIT_FP_HOME}/include -o ${INIT_FP_WRAP} ${INIT_FP_HOME}/src/InitFloorplan.i
  COMMAND ${OPENSTA_HOME}/etc/SwigCleanup.tcl ${INIT_FP_WRAP}
  WORKING_DIRECTORY ${INIT_FP_HOME}/src
  DEPENDS
  ${INIT_FP_HOME}/src/InitFloorplan.i
  ${INIT_FP_HOME}/include/init_fp/InitFloorplan.hh
  )

add_custom_command(OUTPUT ${INIT_FP_TCL_INIT}
  COMMAND ${OPENSTA_HOME}/etc/TclEncode.tcl ${INIT_FP_TCL_INIT} init_fp_tcl_inits InitFloorplan.tcl
  WORKING_DIRECTORY ${INIT_FP_HOME}/src
  DEPENDS InitFloorplan.tcl
  )

add_library(init_fp
  InitFloorplan.cc
  MakeInitFloorplan.cc
  ${INIT_FP_WRAP}
  ${INIT_FP_TCL_INIT}
  )

target_include_directories(init_fp
  PUBLIC ${INIT_FP_HOME}/include
  PRIVATE
  ${OPENROAD_HOME}/include
  ${DBSTA_HOME}/include
  ${OPENDB_HOME}/include
  ${OPENSTA_INCLUDE_DIRS}
  ${TCL_INCLUDE_PATH}
  )
