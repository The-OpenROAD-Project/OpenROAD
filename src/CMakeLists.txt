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

set(OPENSTA_HOME ${PROJECT_SOURCE_DIR}/src/OpenSTA)
set(OPENDB_HOME ${PROJECT_SOURCE_DIR}/src/OpenDB)

add_subdirectory(${OPENDB_HOME})
add_subdirectory(${OPENSTA_HOME})

set(OPENROAD_SOURCE
  StaDb.cc
  DbNetwork.cc
  DbSdcNetwork.cc
  VerilogToDb.cc
  InitFloorplan.cc
  OpenRoad_wrap.cc
  OpenRoadTclInitVar.cc
  )

set(OPENROAD_HEADERS
  StaDb.hh
  DbNetwork.hh
  DbSdcNetwork.hh
  VerilogToDb.hh
  InitFloorplan.hh
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
  ${OPENSTA_HOME}/verilog
  )

set(OPENROAD_TCL_FILES
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
  OpenRoad.tcl
  VerilogToDb.tcl
  InitFloorplan.tcl
  )

# Only used for depends.
set(STA_DB_SWIG_FILES
  OpenRoad.i
  VerilogToDb.i
  InitFloorplan.i
  ${OPENSTA_HOME}/tcl/StaException.i
  ${OPENSTA_HOME}/tcl/StaTcl.i
  ${OPENSTA_HOME}/tcl/NetworkEdit.i
  ${OPENSTA_HOME}/sdf/Sdf.i
  ${OPENSTA_HOME}/dcalc/DelayCalc.i
  ${OPENSTA_HOME}/parasitics/Parasitics.i
  )

add_custom_command(OUTPUT ${OPENROAD_HOME}/src/OpenRoadTclInitVar.cc
  COMMAND ${OPENSTA_HOME}/etc/TclEncode.tcl OpenRoadTclInitVar.cc openroad_tcl_inits ${OPENROAD_TCL_FILES}
  WORKING_DIRECTORY ${OPENROAD_HOME}/src
  DEPENDS ${OPENROAD_TCL_FILES} ${OPENSTA_HOME}/etc/TclEncode.tcl
  )

include(FindSWIG)
add_custom_command(OUTPUT ${OPENROAD_HOME}/src/OpenRoad_wrap.cc
  COMMAND ${SWIG_EXECUTABLE} -tcl8 -c++ -namespace -prefix sta -I${OPENSTA_HOME}/tcl -I${OPENSTA_HOME}/dcalc -I${OPENSTA_HOME}/parasitics -I${OPENSTA_HOME}/sdf -o OpenRoad_wrap.cc OpenRoad.i
  COMMAND ${OPENSTA_HOME}/etc/SwigCleanup.tcl OpenRoad_wrap.cc
  WORKING_DIRECTORY ${OPENROAD_HOME}/src
  DEPENDS ${STA_DB_SWIG_FILES}
  )

################################################################

add_library(StaDb
  ${OPENROAD_SOURCE}
  )

target_include_directories(StaDb
  PUBLIC ${OPENROAD_HOME}/include
  PRIVATE ${STA_INCLUDE_DIRS} ${OPENDB_HOME}/include
  )

target_compile_features(StaDb
  PUBLIC cxx_auto_type
  )

target_compile_options(StaDb PUBLIC ${STA_COMPILE_OPTIONS})

################################################################

add_executable(openroad
  Main.cc
  OpenRoad.cc
  )

target_include_directories(openroad
  PUBLIC ${OPENROAD_HOME}/include
  PRIVATE ${STA_INCLUDE_DIRS} ${OPENDB_HOME}/include
  )

target_link_libraries(openroad
  StaDb
  OpenSTA
  opendb 
  zutil 
  zlib 
  tm
  defin
  defout
  lefin
  lefout
  zutil
  ${TCL_LIB}
  )

if (ZLIB_FOUND)
  target_link_libraries(openroad ${ZLIB_LIBRARIES})
endif()

target_compile_options(openroad PUBLIC ${COMPILE_OPTIONS})

################################################################
# Install
# cmake .. -DCMAKE_INSTALL_PREFIX=<prefix_path>

# executable
install(TARGETS openroad DESTINATION bin)

################################################################

add_custom_target(openroad_tags etags -o TAGS ${OPENROAD_SOURCE} ${OPENROAD_HEADERS} ${OPENROAD_TCL_FILES}
  WORKING_DIRECTORY ${OPENROAD_HOME}/src
  DEPENDS ${OPENROAD_SOURCE} ${OPENROAD_HEADERS} ${OPENROAD_TCL_FILES}
  )
