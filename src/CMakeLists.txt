# OpenStaDB, OpenSTA on OpenDB
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

set(OPENSTA_DB_SOURCE
  OpenStaDB.cc
  OpenStaDB_wrap.cc
  OpenStaDBTclInitVar.cc
  OpenDBNetwork.cc
  )

set(OPENSTA_DB_HEADERS
  OpenStaDB.hh
  OpenDBNetwork.hh
  )

set(STA_INCLUDE_DIRS
  ${OPENSTA_HOME}/util
  ${OPENSTA_HOME}/liberty
  ${OPENSTA_HOME}/network
  ${OPENSTA_HOME}/graph
  ${OPENSTA_HOME}/sdc
  ${OPENSTA_HOME}/parasitics
  ${OPENSTA_HOME}/dcalc
  ${OPENSTA_HOME}/sdf
  ${OPENSTA_HOME}/search
  )

set(OPENSTA_DB_TCL_FILES
  ${OPENSTA_HOME}/tcl/Util.tcl
  ${OPENSTA_HOME}/tcl/Graph.tcl
  ${OPENSTA_HOME}/tcl/Liberty.tcl
  ${OPENSTA_HOME}/tcl/Link.tcl
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
  OpenStaDB.tcl
  )

set(OPENSTA_DB_SWIG_FILES
  OpenStaDB.i
  ${OPENSTA_HOME}/tcl/StaException.i
  ${OPENSTA_HOME}/tcl/StaTcl.i
  ${OPENSTA_HOME}/tcl/NetworkEdit.i
  ${OPENSTA_HOME}/sdf/Sdf.i
  ${OPENSTA_HOME}/dcalc/DelayCalc.i
  ${OPENSTA_HOME}/parasitics/Parasitics.i
  )

add_custom_command(OUTPUT ${OPENSTA_DB_HOME}/src/OpenStaDBTclInitVar.cc
  COMMAND ${OPENSTA_HOME}/etc/TclEncode.tcl OpenStaDBTclInitVar.cc opensta_db_tcl_inits ${OPENSTA_DB_TCL_FILES}
  WORKING_DIRECTORY ${OPENSTA_DB_HOME}/src
  DEPENDS ${OPENSTA_DB_TCL_FILES} ${OPENSTA_HOME}/etc/TclEncode.tcl
  )

include(FindSWIG)
add_custom_command(OUTPUT ${OPENSTA_DB_HOME}/src/OpenStaDB_wrap.cc
  COMMAND ${SWIG_EXECUTABLE} -tcl8 -c++ -namespace -prefix sta -I${OPENSTA_HOME}/tcl -I${OPENSTA_HOME}/dcalc -I${OPENSTA_HOME}/parasitics -I${OPENSTA_HOME}/sdf -o OpenStaDB_wrap.cc OpenStaDB.i
  COMMAND ${OPENSTA_HOME}/etc/SwigCleanup.tcl OpenStaDB_wrap.cc
  WORKING_DIRECTORY ${OPENSTA_DB_HOME}/src
  DEPENDS ${OPENSTA_DB_SWIG_FILES}
  )

################################################################

add_library(OpenStaDB
  ${OPENSTA_DB_SOURCE}
  )

target_include_directories(OpenStaDB
  PUBLIC ${OPENSTA_DB_HOME}/include
  PRIVATE ${STA_INCLUDE_DIRS} ${OPENDB_HOME}/include
  )

target_compile_features(OpenStaDB
  PUBLIC cxx_auto_type
  )

target_compile_options(OpenStaDB PUBLIC ${STA_COMPILE_OPTIONS})

################################################################

add_executable(opensta_db
  Main.cc
  OpenRoad.cc
  )

target_include_directories(opensta_db
  PUBLIC ${OPENSTA_DB_HOME}/include
  PRIVATE ${STA_INCLUDE_DIRS} ${OPENDB_HOME}/include
  )

target_link_libraries(opensta_db
  OpenStaDB
  OpenSTA
  opendb 
  zutil 
  zlib 
  tm
  lef
  lefrw
  defin
  defout
  lefin
  lefout
  zutil
  ${TCL_LIB}
  )

if (ZLIB_FOUND)
  target_link_libraries(opensta_db ${ZLIB_LIBRARIES})
endif()

target_compile_options(opensta_db PUBLIC ${COMPILE_OPTIONS})

################################################################
# Install
# cmake .. -DCMAKE_INSTALL_PREFIX=<prefix_path>

# executable
install(TARGETS opensta_db DESTINATION bin)

################################################################

add_custom_target(opensta_db_tags etags -o TAGS ${OPENSTA_DB_SOURCE} ${OPENSTA_DB_HEADERS} ${OPENSTA_DB_TCL_FILES}
  WORKING_DIRECTORY ${OPENSTA_DB_HOME}
  DEPENDS ${OPENSTA_DB_SOURCE} ${OPENSTA_DB_HEADERS} ${OPENSTA_DB_TCL_FILES}
  )
