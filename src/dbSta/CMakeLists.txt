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

# Only used for depends.
set(DBSTA_SWIG_FILES
  dbSta.i
  ${OPENSTA_HOME}/tcl/StaException.i
  ${OPENSTA_HOME}/tcl/StaTcl.i
  ${OPENSTA_HOME}/tcl/NetworkEdit.i
  ${OPENSTA_HOME}/sdf/Sdf.i
  ${OPENSTA_HOME}/dcalc/DelayCalc.i
  ${OPENSTA_HOME}/parasitics/Parasitics.i
  )

set(DBSTA_WRAP ${CMAKE_CURRENT_BINARY_DIR}/dbSta_wrap.cc)

add_custom_command(OUTPUT ${DBSTA_WRAP}
  COMMAND ${SWIG_EXECUTABLE} -tcl8 -c++ -namespace -prefix sta -I${OPENSTA_HOME}/tcl -I${OPENSTA_HOME}/dcalc -I${OPENSTA_HOME}/parasitics -I${OPENSTA_HOME}/sdf -o ${DBSTA_WRAP} dbSta.i
  COMMAND ${OPENSTA_HOME}/etc/SwigCleanup.tcl ${DBSTA_WRAP}
  WORKING_DIRECTORY ${OPENROAD_HOME}/src/dbSta
  DEPENDS ${DBSTA_SWIG_FILES}
  )

add_library(dbSta
  dbSta.cc
  dbNetwork.cc
  dbSdcNetwork.cc
  ${DBSTA_WRAP}
  )

target_include_directories(dbSta
  PUBLIC ${OPENROAD_HOME}/include
  PRIVATE ${STA_INCLUDE_DIRS} ${OPENDB_HOME}/include
  )
