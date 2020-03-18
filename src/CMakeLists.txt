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
set(INIT_FP_HOME ${PROJECT_SOURCE_DIR}/src/init_fp)
set(DBSTA_HOME ${PROJECT_SOURCE_DIR}/src/dbSta)
set(RESIZER_HOME ${PROJECT_SOURCE_DIR}/src/resizer)
set(OPENDP_HOME ${PROJECT_SOURCE_DIR}/src/opendp)
set(REPLACE_HOME ${PROJECT_SOURCE_DIR}/src/replace)
set(PDNGEN_HOME ${OPENROAD_HOME}/src/pdngen)
set(ICEWALL_HOME ${OPENROAD_HOME}/src/ICeWall)
set(IOPLACER_HOME ${PROJECT_SOURCE_DIR}/src/ioPlacer)
set(TRITONCTS_HOME ${PROJECT_SOURCE_DIR}/src/TritonCTS) 
set(FASTROUTE_HOME ${PROJECT_SOURCE_DIR}/src/FastRoute)
set(TAPCELL_HOME ${OPENROAD_HOME}/src/tapcell)
set(OPENRCX_HOME ${OPENROAD_HOME}/src/OpenRCX)
set(MPLACE_HOME ${PROJECT_SOURCE_DIR}/src/TritonMacroPlace)
set(OPENPHYSYN_HOME ${PROJECT_SOURCE_DIR}/src/OpenPhySyn)
set(PDNSIM_HOME ${PROJECT_SOURCE_DIR}/src/PDNSim)

set(OPENROAD_TCL_INIT ${CMAKE_CURRENT_BINARY_DIR}/OpenRoadTclInitVar.cc)

set(OPENROAD_SOURCE
  OpenRoad.cc
  Error.cc
  ${OPENROAD_TCL_INIT}
  Main.cc
  )

set(OPENROAD_TCL_FILES
  ${OPENSTA_HOME}/tcl/Util.tcl
  OpenRoad.tcl
  ${ICEWALL_HOME}/src/ICeWall.tcl
  ${PDNGEN_HOME}/src/PdnGen.tcl
  )

add_custom_command(OUTPUT ${OPENROAD_TCL_INIT}
  COMMAND ${OPENSTA_HOME}/etc/TclEncode.tcl ${OPENROAD_TCL_INIT} openroad_tcl_inits ${OPENROAD_TCL_FILES}
  WORKING_DIRECTORY ${OPENROAD_HOME}/src
  DEPENDS ${OPENROAD_TCL_FILES} ${OPENSTA_HOME}/etc/TclEncode.tcl
  )

################################################################

# OSX tcl is deprecated and prints a warning, so look for a user installed
# version before using the system version.
# I tried to override the library search order instead but failed.
# CMAKE_FIND_FRAMEWORK LAST bypasses the version in the framework directory
# but not the one in /usr/lib.
# This calls cmake/FindTCL.cmake
# Do not use REQUIRED because it also requires TK.
find_package(TCL)

find_package(SWIG REQUIRED 3.0)
include(UseSWIG)
set(OPENDB_SWIG_WARNINGS "-w509,503,501,472,467,402,401,317,325,378,383,389,365,362,314,258,240,203,201")

find_package(Boost)

# OpenDB's swig wrapper is not in the library (and mistakenly exports
# to the global namespace) so make our own.
# build/src/CMakeFiles/opendb_swig.dir/opendbtclTCL_wrap.cxx
set(OPENDB_SWIG_FILE ${OPENDB_HOME}/src/swig/tcl/opendbtcl.i)

set_property(SOURCE ${OPENDB_SWIG_FILE}
  PROPERTY CPLUSPLUS ON
)

# Ubuntu 18.04 cmake version 3.10.2 does not support the
# COMPILE_OPTIONS and INCLUDE_DIRECTORIES properties, so cram
# them into SWIG_FLAGS for the time being.
set_property(SOURCE ${OPENDB_SWIG_FILE}
  PROPERTY SWIG_FLAGS
  -module opendbtcl
  -namespace -prefix odb
  ${OPENDB_SWIG_WARNINGS}
  -I${OPENDB_HOME}/include/opendb
  -I${OPENDB_HOME}/src/swig/tcl
)

set_property(SOURCE ${OPENDB_SWIG_FILE}
  PROPERTY DEPENDS
  Exception.i
)

swig_add_library(opendb_swig
  LANGUAGE tcl
  TYPE STATIC
  SOURCES ${OPENDB_SWIG_FILE}
)

target_include_directories(opendb_swig
  PRIVATE
  ${Boost_INCLUDE_DIRS}
  ${TCL_INCLUDE_PATH}
)

swig_link_libraries(opendb_swig
  PUBLIC
  opendb
)

################################################################

set(OPENROAD_SWIG_FILE OpenRoad.i)

set_property(SOURCE ${OPENROAD_SWIG_FILE}
  PROPERTY CPLUSPLUS ON
)

# Ubuntu 18.04 cmake version 3.10.2 does not support the
# COMPILE_OPTIONS and INCLUDE_DIRECTORIES properties, so cram
# them into SWIG_FLAGS for the time being.
set_property(SOURCE ${OPENROAD_SWIG_FILE}
  PROPERTY SWIG_FLAGS
  -module openroad
  -namespace -prefix ord
  -I${DBSTA_HOME}/include
  -I${OPENROAD_HOME}/src
  -I${OPENDB_HOME}/include
)

set_property(SOURCE ${OPENROAD_SWIG_FILE}
  PROPERTY DEPENDS
  Exception.i
)

swig_add_library(openroad_swig
  LANGUAGE tcl
  TYPE STATIC
  SOURCES ${OPENROAD_SWIG_FILE}
)

# build/src/CMakeFiles/openroad_swig.dir/OpenRoadTCL_wrap.cxx
swig_link_libraries(openroad_swig
  PUBLIC
  openroad
  # OpenDB public includes incorrectly point to include/opendb so this fails.
  #opendb
)

target_include_directories(openroad_swig
  PUBLIC
  ${OPENROAD_HOME}/include

  PRIVATE
  ${OPENROAD_HOME}/src
  ${OPENSTA_HOME}/util
  ${OPENSTA_HOME}/sdc
  ${OPENSTA_HOME}/network
  ${OPENSTA_HOME}/parasitics
  ${OPENSTA_HOME}/liberty
  ${OPENSTA_HOME}/graph
  ${OPENSTA_HOME}/search

  ${DBSTA_HOME}/include
  ${OPENDB_HOME}/include

  ${TCL_INCLUDE_PATH}
)

################################################################
#
# Library dependencies
#
################################################################

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

set(OPENSTA_INCLUDE_DIRS
  ${OPENSTA_HOME}/util
  ${OPENSTA_HOME}/liberty
  ${OPENSTA_HOME}/network
  ${OPENSTA_HOME}/graph
  ${OPENSTA_HOME}/sdc
  ${OPENSTA_HOME}/parasitics
  ${OPENSTA_HOME}/dcalc
  ${OPENSTA_HOME}/search
  ${OPENSTA_HOME}/app
  )

# Build flow tools
add_subdirectory(init_fp)
add_subdirectory(OpenDB)
add_subdirectory(OpenSTA)
add_subdirectory(dbSta)
add_subdirectory(resizer)
add_subdirectory(flute3)
add_subdirectory(replace)
add_subdirectory(opendp)
add_subdirectory(ioPlacer)
add_subdirectory(TritonCTS)
add_subdirectory(FastRoute)
add_subdirectory(tapcell)
add_subdirectory(TritonMacroPlace)
add_subdirectory(OpenRCX)
add_subdirectory(OpenPhySyn)
add_subdirectory(PDNSim)

################################################################

add_executable(openroad
  ${OPENROAD_SOURCE}
  )

target_compile_options(openroad
  PRIVATE
  $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Wcast-qual -Wredundant-decls -Wformat-security>
  $<$<CXX_COMPILER_ID:AppleClang>:-Wall -Wextra -pedantic -Wcast-qual -Wredundant-decls -Wformat-security>
  $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Wcast-qual -Wredundant-decls -Wformat-security>
  )

target_include_directories(openroad
  PUBLIC ${OPENROAD_HOME}/include
  PRIVATE
  ${OPENROAD_HOME}/src
  ${INIT_FP_HOME}/include
  ${OPENDB_HOME}/include
  ${REPLACE_HOME}/include
  ${OPENDP_HOME}/include
  ${MPLACE_HOME}/include
  ${FASTROUTE_HOME}/include
  ${OPENPHYSYN_HOME}/include
  flute3
  ${PDNGEN_HOME}/include
  ${TAPCELL_HOME}/include
  ${OPENRCX_HOME}/include
  ${PDNSIM_HOME}/include
  ${TCL_INCLUDE_PATH}
  ${Boost_INCLUDE_DIRS}
  )

target_link_libraries(openroad
  init_fp
  openroad_swig
  replace
  opendp
  resizer
  ioPlacer
  flute
  dbSta
  OpenSTA
  opendb
  opendb_swig
  OpenRCX
  TritonCTS
  FastRoute
  tapcell
  TritonMacroPlace
  ParquetFP
  ABKCommon
  OpenPhySyn
  pdnsim
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

################################################################

add_custom_target(openroad_tags etags -o TAGS
  ${OPENROAD_SOURCE}
  ${OPENROAD_HOME}/include/openroad/*.hh
  ${OPENROAD_HOME}/src/dbSta/src/*.hh
  ${DBSTA_HOME}/include/db_sta/*.hh
  ${OPENROAD_HOME}/src/dbSta/src/*.cc
  ${RESIZER_HOME}/src/*.hh
  ${RESIZER_HOME}/src/*.cc
  WORKING_DIRECTORY ${OPENROAD_HOME}/src
  DEPENDS ${OPENROAD_SOURCE} ${OPENROAD_HEADERS} ${OPENROAD_TCL_FILES}
  )
