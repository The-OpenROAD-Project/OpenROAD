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

set(OPENROAD_WRAP ${CMAKE_CURRENT_BINARY_DIR}/OpenRoad_wrap.cc)
set(OPENDB_WRAP ${CMAKE_CURRENT_BINARY_DIR}/OpenDB_wrap.cc)
set(OPENROAD_TCL_INIT ${CMAKE_CURRENT_BINARY_DIR}/OpenRoadTclInitVar.cc)

set(OPENROAD_SOURCE
  Main.cc

  dbReadVerilog.cc
  InitFloorplan.cc

  OpenRoad.cc
  ${OPENROAD_WRAP}
  ${OPENDB_WRAP}
  ${OPENROAD_TCL_INIT}
  )

set(OPENROAD_TCL_FILES
  ${OPENSTA_HOME}/tcl/Util.tcl
  OpenRoad.tcl
  dbReadVerilog.tcl
  InitFloorplan.tcl
  resizer/src/Resizer.tcl
  )

add_custom_command(OUTPUT ${OPENROAD_TCL_INIT}
  COMMAND ${OPENSTA_HOME}/etc/TclEncode.tcl ${OPENROAD_TCL_INIT} openroad_tcl_inits ${OPENROAD_TCL_FILES}
  WORKING_DIRECTORY ${OPENROAD_HOME}/src
  DEPENDS ${OPENROAD_TCL_FILES} ${OPENSTA_HOME}/etc/TclEncode.tcl
  )

################################################################

include(FindSWIG)

add_custom_command(OUTPUT ${OPENDB_WRAP}
  COMMAND ${SWIG_EXECUTABLE} -tcl8 -c++ -namespace -prefix odb -I${OPENDB_HOME}/include/opendb -I${OPENDB_HOME}/src/swig/tcl -o ${OPENDB_WRAP} ${OPENDB_HOME}/src/swig/tcl/opendbtcl.i
  COMMAND ${OPENSTA_HOME}/etc/SwigCleanup.tcl ${OPENDB_WRAP}
  WORKING_DIRECTORY ${OPENROAD_HOME}/src
  DEPENDS ${OPENDB_HOME}/src/swig/tcl/opendbtcl.i
  )

add_custom_command(OUTPUT ${OPENROAD_WRAP}
  COMMAND ${SWIG_EXECUTABLE} -tcl8 -c++ -namespace -prefix sta -I${OPENROAD}/src -o ${OPENROAD_WRAP} OpenRoad.i
  COMMAND ${OPENSTA_HOME}/etc/SwigCleanup.tcl ${OPENROAD_WRAP}
  WORKING_DIRECTORY ${OPENROAD_HOME}/src
  DEPENDS OpenRoad.i InitFloorplan.i
  )

################################################################

add_subdirectory(OpenDB)
add_subdirectory(OpenSTA)
add_subdirectory(dbSta)
add_subdirectory(resizer)
add_subdirectory(flute3)
add_subdirectory(replace)

################################################################

add_executable(openroad
  ${OPENROAD_SOURCE}
  )

target_include_directories(openroad
  PUBLIC ${OPENROAD_HOME}/include
  PRIVATE
  ${OPENROAD_HOME}/src
  ${OPENDB_HOME}/include
  flute3
  )

target_link_libraries(openroad
  replace
  resizer
  flute
  dbSta
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

add_custom_target(openroad_tags etags -o TAGS
  ${OPENROAD_SOURCE}
  ${OPENROAD_HOME}/include/openroad/*.hh
  ${OPENROAD_HOME}/src/dbSta/src/*.cc
  ${OPENROAD_HOME}/src/dbSta/src/*.hh
  ${OPENROAD_HOME}/src/dbSta/src/*.hh
  ${OPENROAD_HOME}/include/db_sta/*.hh
  ${OPENROAD_HOME}/src/resizer/src/*.cc
  ${OPENROAD_HOME}/src/resizer/src/*.hh
  WORKING_DIRECTORY ${OPENROAD_HOME}/src
  DEPENDS ${OPENROAD_SOURCE} ${OPENROAD_HEADERS} ${OPENROAD_TCL_FILES}
  )
