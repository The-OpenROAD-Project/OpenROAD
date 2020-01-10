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
set(OPENDB_WRAP ${CMAKE_CURRENT_BINARY_DIR}/OpenDB_wrap.cc)

set(RESIZER_HOME ${PROJECT_SOURCE_DIR}/src/resizer)
set(OPENDP_HOME ${PROJECT_SOURCE_DIR}/src/opendp)

set(PDNGEN_HOME ${OPENROAD_HOME}/src/pdngen)

set(IOPLACER_HOME ${PROJECT_SOURCE_DIR}/src/ioPlacer)

set(TRITONCTS_HOME ${PROJECT_SOURCE_DIR}/src/TritonCTS) 

set(FASTROUTE_HOME ${PROJECT_SOURCE_DIR}/src/FastRoute)
set(TAPCELL_HOME ${OPENROAD_HOME}/src/tapcell)

set(MPLACE_HOME ${PROJECT_SOURCE_DIR}/src/TritonMacroPlace)

set(OPENROAD_WRAP ${CMAKE_CURRENT_BINARY_DIR}/OpenRoad_wrap.cc)
set(OPENROAD_TCL_INIT ${CMAKE_CURRENT_BINARY_DIR}/OpenRoadTclInitVar.cc)

set(OPENROAD_SOURCE
  OpenRoad.cc
  ${OPENROAD_WRAP}
  ${OPENROAD_TCL_INIT}
  Main.cc

  dbReadVerilog.cc
  InitFloorplan.cc
  InitFlute.cc

  ${OPENDB_WRAP}
  )

set(OPENROAD_TCL_FILES
  ${OPENSTA_HOME}/tcl/Util.tcl
  OpenRoad.tcl
  dbReadVerilog.tcl
  InitFloorplan.tcl
  )

add_custom_command(OUTPUT ${OPENROAD_TCL_INIT}
  COMMAND ${OPENSTA_HOME}/etc/TclEncode.tcl ${OPENROAD_TCL_INIT} openroad_tcl_inits ${OPENROAD_TCL_FILES}
  WORKING_DIRECTORY ${OPENROAD_HOME}/src
  DEPENDS ${OPENROAD_TCL_FILES} ${OPENSTA_HOME}/etc/TclEncode.tcl
  )

################################################################

set(SWIG_REQUIRED_VERSION "3.0")
find_package(SWIG REQUIRED)
set(SWIG_WARNINGS "-w509,503,501,472,467,402,401,317,325,378,383,389,365,362,314,258,240,203,201")

# OpenDB's swig wrapper is not in the library (and mistakenly exports
# to the global namespace) so make our own.
add_custom_command(OUTPUT ${OPENDB_WRAP}
  COMMAND ${SWIG_EXECUTABLE} -DSWIG_DIRECTOR_STATIC -tcl8 -c++ ${SWIG_WARNINGS} -namespace -prefix odb -I${OPENDB_HOME}/include/opendb -I${OPENDB_HOME}/src/swig/tcl -o ${OPENDB_WRAP} ${OPENDB_HOME}/src/swig/tcl/opendbtcl.i
  COMMAND ${OPENSTA_HOME}/etc/SwigCleanup.tcl ${OPENDB_WRAP}
  WORKING_DIRECTORY ${OPENROAD_HOME}/src
  DEPENDS
  ${OPENDB_HOME}/src/swig/tcl/opendbtcl.i
  ${OPENDB_HOME}/src/swig/tcl/dbenums.i
  ${OPENDB_HOME}/src/swig/tcl/parserenums.i
  ${OPENDB_HOME}/src/swig/tcl/dbhelpers.i
  ${OPENDB_HOME}/src/swig/tcl/dbdiff.i
  )

add_custom_command(OUTPUT ${OPENROAD_WRAP}
  COMMAND ${SWIG_EXECUTABLE} -tcl8 -c++ -namespace -prefix ord -I${OPENROAD}/src -o ${OPENROAD_WRAP} OpenRoad.i
  COMMAND ${OPENSTA_HOME}/etc/SwigCleanup.tcl ${OPENROAD_WRAP}
  WORKING_DIRECTORY ${OPENROAD_HOME}/src
  DEPENDS OpenRoad.i InitFloorplan.i
  )

################################################################
#
# Library dependencies
#
################################################################

# OSX tcl is deprecated and prints a warning, so look for a user installed
# version before using the system version.
# I tried to override the library search order instead but failed.
# CMAKE_FIND_FRAMEWORK LAST bypasses the version in the framework directory
# but not the one in /usr/lib.
# This calls cmake/FindTCL.cmake
# Do not use REQUIRED because it also requires TK.
find_package(TCL)

# Zlib
include(FindZLIB)
# Translate cmake bool to StaConfig.h ifdef bool
if (ZLIB_FOUND)
  set(ZLIB 1)
else()
  set(ZLIB 0)
endif()

################################################################

# Disable OpenDB stand-alone executable builds.
set(BUILD_PYTHON false)
set(BUILD_TCL false)

# Make the tools aware that they are being built as part of the top-level app.
# This allows the tool to have any custom CMake/C++ configuration for the top-level built
set(OPENROAD_BUILD ON)
add_definitions(-DOPENROAD_BUILD)

# Build flow tools
add_subdirectory(OpenDB)
add_subdirectory(OpenSTA)
add_subdirectory(dbSta)
add_subdirectory(resizer)
add_subdirectory(flute3)
add_subdirectory(replace)
add_subdirectory(opendp)
add_subdirectory(ioPlacer)
add_subdirectory(pdngen)
add_subdirectory(TritonCTS)
add_subdirectory(FastRoute)
add_subdirectory(tapcell)
add_subdirectory(TritonMacroPlace)

################################################################

add_executable(openroad
  ${OPENROAD_SOURCE}
  )

target_include_directories(openroad
  PUBLIC ${OPENROAD_HOME}/include
  PRIVATE
  ${OPENROAD_HOME}/src
  ${OPENDB_HOME}/include
  ${OPENDP_HOME}/include
  ${MPLACE_HOME}/include
  ${FASTROUTE_HOME}/include
  flute3
  ${PDNGEN_HOME}/include
  ${TAPCELL_HOME}/include
  SYSTEM
  ${TCL_INCLUDE_PATH}
  )

target_link_libraries(openroad
  pdngen
  replace
  opendp
  resizer
  ioPlacer
  flute
  dbSta
  OpenSTA
  opendb
  TritonCTS
  FastRoute
  tapcell
  TritonMacroPlace
  ParquetFP
  ABKCommon
  zlib
  tm
  defin
  defout
  lefin
  lefout
  zutil
  ${TCL_LIBRARY}
)

if (ZLIB_FOUND)
  target_link_libraries(openroad ${ZLIB_LIBRARIES})
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_link_libraries(openroad -pthread)
endif()

# Ancient versions of glibc found on CentOS required librt.
# OSX has an empty version of librt that causes a link error,
# so this is protected from linking there.
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
    AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
  find_library(LIBRT rt)
  target_link_libraries(openroad ${LIBRT})
endif()

target_compile_options(openroad PUBLIC ${COMPILE_OPTIONS})

################################################################
# Install
# cmake .. -DCMAKE_INSTALL_PREFIX=<prefix_path>

# executable
install(TARGETS openroad DESTINATION bin)
# etc magic flute files
install(FILES ../etc/POST9.dat ../etc/POWV9.dat DESTINATION etc)

################################################################

add_custom_target(openroad_tags etags -o TAGS
  ${OPENROAD_SOURCE}
  ${OPENROAD_HOME}/include/openroad/*.hh
  ${OPENROAD_HOME}/src/dbSta/src/*.hh
  ${OPENROAD_HOME}/include/db_sta/*.hh
  ${OPENROAD_HOME}/src/dbSta/src/*.cc
  ${RESIZER_HOME}/src/*.hh
  ${RESIZER_HOME}/src/*.cc
  WORKING_DIRECTORY ${OPENROAD_HOME}/src
  DEPENDS ${OPENROAD_SOURCE} ${OPENROAD_HEADERS} ${OPENROAD_TCL_FILES}
  )
