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

set(DBSTA_TCL_INIT ${CMAKE_CURRENT_BINARY_DIR}/dbStaTclInitVar.cc)
set(DBSTA_WRAP ${CMAKE_CURRENT_BINARY_DIR}/dbSta_wrap.cc)

# Only used for depends.
set(DBSTA_SWIG_FILES
  ${DBSTA_HOME}/src/dbSta.i

  ${OPENSTA_HOME}/tcl/Exception.i
  ${OPENSTA_HOME}/tcl/StaTcl.i
  ${OPENSTA_HOME}/tcl/NetworkEdit.i
  ${OPENSTA_HOME}/sdf/Sdf.i
  ${OPENSTA_HOME}/dcalc/DelayCalc.i
  ${OPENSTA_HOME}/parasitics/Parasitics.i
  )

set(DBSTA_TCL_FILES
  ${OPENSTA_HOME}/tcl/Graph.tcl
  ${OPENSTA_HOME}/tcl/Liberty.tcl
  ${OPENSTA_HOME}/tcl/Network.tcl
  ${OPENSTA_HOME}/tcl/NetworkEdit.tcl
  ${OPENSTA_HOME}/tcl/Sdc.tcl
  ${OPENSTA_HOME}/tcl/Search.tcl
  ${OPENSTA_HOME}/tcl/Cmds.tcl
  ${OPENSTA_HOME}/tcl/Variables.tcl
  ${OPENSTA_HOME}/tcl/Sta.tcl
  ${OPENSTA_HOME}/tcl/Power.tcl
  ${OPENSTA_HOME}/tcl/Splash.tcl
  ${OPENSTA_HOME}/dcalc/DelayCalc.tcl
  ${OPENSTA_HOME}/parasitics/Parasitics.tcl
  ${OPENSTA_HOME}/sdf/Sdf.tcl

  ${DBSTA_HOME}/src/dbReadVerilog.tcl
  )

set(OPENSTA_INCLUDE_DIRS
  ${OPENSTA_HOME}/util
  ${OPENSTA_HOME}/liberty
  ${OPENSTA_HOME}/network
  ${OPENSTA_HOME}/graph
  ${OPENSTA_HOME}/sdc
  ${OPENSTA_HOME}/parasitics
  ${OPENSTA_HOME}/dcalc
  ${OPENSTA_HOME}/sdf
  ${OPENSTA_HOME}/search
  ${OPENSTA_HOME}/verilog
  ${OPENSTA_HOME}/app
  )

add_custom_command(OUTPUT ${DBSTA_TCL_INIT}
  COMMAND ${OPENSTA_HOME}/etc/TclEncode.tcl ${DBSTA_TCL_INIT} dbsta_tcl_inits ${DBSTA_TCL_FILES}
  DEPENDS ${DBSTA_TCL_FILES} ${OPENSTA_HOME}/etc/TclEncode.tcl
  )

add_custom_command(OUTPUT ${DBSTA_WRAP}
  COMMAND ${SWIG_EXECUTABLE} -tcl8 -c++ -namespace -prefix sta -I${OPENSTA_HOME}/tcl -I${OPENSTA_HOME}/dcalc -I${OPENSTA_HOME}/parasitics -I${OPENSTA_HOME}/sdf -o ${DBSTA_WRAP} ${DBSTA_HOME}/src/dbSta.i
  COMMAND ${OPENSTA_HOME}/etc/SwigCleanup.tcl ${DBSTA_WRAP}
  WORKING_DIRECTORY ${OPENROAD_HOME}/src/dbSta
  DEPENDS ${DBSTA_SWIG_FILES}
  )

add_library(dbSta
  dbSta.cc
  dbNetwork.cc
  dbSdcNetwork.cc
  ${DBSTA_TCL_INIT}
  ${DBSTA_WRAP}
  dbReadVerilog.cc
  )

target_include_directories(dbSta
  PUBLIC
  ${OPENROAD_HOME}/include
  ${DBSTA_HOME}/include
  PRIVATE
  ${OPENSTA_INCLUDE_DIRS} ${OPENDB_HOME}/include
  ${TCL_INCLUDE_PATH}
  )
