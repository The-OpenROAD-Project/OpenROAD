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
set(FASTROUTE_HOME ${PROJECT_SOURCE_DIR}/src/FastRoute)

set(OPENPHYSYN_HOME ${PROJECT_SOURCE_DIR}/src/OpenPhySyn)

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

include(FindSWIG)

add_custom_command(OUTPUT ${OPENDB_WRAP}
  COMMAND ${SWIG_EXECUTABLE} -DSWIG_DIRECTOR_STATIC -tcl8 -c++ -namespace -prefix odb -I${OPENDB_HOME}/include/opendb -I${OPENDB_HOME}/src/swig/tcl -o ${OPENDB_WRAP} ${OPENDB_HOME}/src/swig/tcl/opendbtcl.i
  COMMAND ${OPENSTA_HOME}/etc/SwigCleanup.tcl ${OPENDB_WRAP}
  WORKING_DIRECTORY ${OPENROAD_HOME}/src
  DEPENDS ${OPENDB_HOME}/src/swig/tcl/opendbtcl.i
  )

add_custom_command(OUTPUT ${OPENROAD_WRAP}
  COMMAND ${SWIG_EXECUTABLE} -tcl8 -c++ -namespace -prefix ord -I${OPENROAD}/src -o ${OPENROAD_WRAP} OpenRoad.i
  COMMAND ${OPENSTA_HOME}/etc/SwigCleanup.tcl ${OPENROAD_WRAP}
  WORKING_DIRECTORY ${OPENROAD_HOME}/src
  DEPENDS OpenRoad.i InitFloorplan.i
  )

################################################################


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
#
# Locate TCL library.
#
# Note that the cmake findTcl module is hopeless for OSX
# because there doesn't appear to be a way to override
# searching OSX system directories before unix directories.

set(TCL_POSSIBLE_NAMES tcl87 tcl8.7
  tcl86 tcl8.6
  tcl85 tcl8.5
  tcl84 tcl8.4
  tcl83 tcl8.3
  tcl82 tcl8.2
  )

# tcl lib path guesses.
if (NOT TCL_LIB_PATHS)
  if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(TCL_LIB_PATHS /usr/local/lib)
    set(TCL_NO_DEFAULT_PATH TRUE)
  endif()
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(TCL_LIB_PATHS /usr/lib
    /usr/local/lib
    )
  set(TCL_NO_DEFAULT_PATH FALSE)
endif()

if (NOT TCL_LIB)
  # bagbiter cmake doesn't have a way to pass NO_DEFAULT_PATH as a parameter.
  if (TCL_NO_DEFAULT_PATH)
    find_library(TCL_LIB
      NAMES tcl ${TCL_POSSIBLE_NAMES}
      PATHS ${TCL_LIB_PATHS}
      NO_DEFAULT_PATH
      )
  else()
    find_library(TCL_LIB
      NAMES tcl ${TCL_POSSIBLE_NAMES}
      PATHS ${TCL_LIB_PATHS}
      )
  endif()
endif()
message(STATUS "TCL lib: ${TCL_LIB}")

get_filename_component(TCL_LIB_DIR "${TCL_LIB}" PATH)
get_filename_component(TCL_LIB_PARENT1 "${TCL_LIB_DIR}" PATH)
get_filename_component(TCL_LIB_PARENT2 "${TCL_LIB_PARENT1}" PATH)

# Locate tcl.h
if (NOT TCL_HEADER)
  find_file(TCL_HEADER tcl.h
    PATHS ${TCL_LIB_PARENT1} ${TCL_LIB_PARENT2}
    PATH_SUFFIXES include include/tcl
    NO_DEFAULT_PATH
    )
endif()
message(STATUS "TCL header: ${TCL_HEADER}")
get_filename_component(TCL_HEADER_DIR "${TCL_HEADER}" PATH)
include_directories(SYSTEM ${TCL_HEADER_DIR})

################################################################

# Disable OpenDB stand-alone executable builds.
# Simplistic findTCL fails on OSX.
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
add_subdirectory(FastRoute)
add_subdirectory(OpenPhySyn)


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
  ${FASTROUTE_HOME}/include
  ${OPENPHYSYN_HOME}/include
  flute3
  ${PDNGEN_HOME}/include
  SYSTEM
  ${TCL_HEADER_DIR}
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
  OpenPhySyn
  FastRoute
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

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_link_libraries(openroad -pthread)
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

