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

set(OPENROAD_SOURCE
  Main.cc
  OpenRoad.cc
  OpenSTA_wrap.cc
  OpenDB_wrap.cc
  OpenRoadTclInitVar.cc
  dbSta.cc
  dbNetwork.cc
  dbSdcNetwork.cc
  dbReadVerilog.cc
  InitFloorplan.cc
  )

set(OPENROAD_HEADERS
  dbSta.hh
  dbNetwork.hh
  dbSdcNetwork.hh
  dbReadVerilog.hh
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
  dbReadVerilog.tcl
  InitFloorplan.tcl
  resizer/Resizer.tcl
  )

# Only used for depends.
set(OPENROAD_SWIG_FILES
  OpenRoad.i
  InitFloorplan.i
  resizer/Resizer.i
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
add_custom_command(OUTPUT ${OPENROAD_HOME}/src/OpenSTA_wrap.cc
  COMMAND ${SWIG_EXECUTABLE} -tcl8 -c++ -namespace -prefix sta -I${OPENSTA_HOME}/tcl -I${OPENSTA_HOME}/dcalc -I${OPENSTA_HOME}/parasitics -I${OPENSTA_HOME}/sdf -o OpenSTA_wrap.cc OpenRoad.i
  COMMAND ${OPENSTA_HOME}/etc/SwigCleanup.tcl OpenSTA_wrap.cc
  WORKING_DIRECTORY ${OPENROAD_HOME}/src
  DEPENDS ${OPENROAD_SWIG_FILES}
  )

add_custom_command(OUTPUT ${OPENROAD_HOME}/src/OpenDB_wrap.cc
  COMMAND ${SWIG_EXECUTABLE} -tcl8 -c++ -namespace -prefix odb -I${OPENDB_HOME}/include/opendb -I${OPENDB_HOME}/src/swig/tcl -o OpenDB_wrap.cc ${OPENDB_HOME}/src/swig/tcl/opendbtcl.i
  COMMAND ${OPENSTA_HOME}/etc/SwigCleanup.tcl OpenDB_wrap.cc
  WORKING_DIRECTORY ${OPENROAD_HOME}/src
  DEPENDS ${OPENDB_HOME}/src/swig/tcl/opendbtcl.i
  )

################################################################

add_subdirectory(OpenDB)
add_subdirectory(OpenSTA)
add_subdirectory(flute3)
add_subdirectory(resizer)

################################################################

add_executable(openroad
  ${OPENROAD_SOURCE}
  )

target_include_directories(openroad
  PUBLIC ${OPENROAD_HOME}/include
  PRIVATE ${STA_INCLUDE_DIRS} ${OPENDB_HOME}/include flute3
  )

target_link_libraries(openroad
  resizer
  flute
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

add_custom_target(openroad_tags etags -o TAGS ${OPENROAD_SOURCE} ${OPENROAD_HEADERS} ${OPENROAD_TCL_FILES} resizer/src/*.cc resizer/src/*.hh
  WORKING_DIRECTORY ${OPENROAD_HOME}/src
  DEPENDS ${OPENROAD_SOURCE} ${OPENROAD_HEADERS} ${OPENROAD_TCL_FILES}
  )
