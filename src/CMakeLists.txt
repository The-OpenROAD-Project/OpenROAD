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

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/src/cmake")

include("openroad")

set(OPENSTA_HOME ${PROJECT_SOURCE_DIR}/src/sta)
set(OPENDB_HOME ${PROJECT_SOURCE_DIR}/src/OpenDB)
set(DBSTA_HOME ${PROJECT_SOURCE_DIR}/src/dbSta)
set(RESIZER_HOME ${PROJECT_SOURCE_DIR}/src/rsz)
set(ICEWALL_HOME ${OPENROAD_HOME}/src/ICeWall)

set(OPENROAD_SOURCE
  OpenRoad.cc
  Main.cc
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

# check for tclReadline
set(TCL_READLINE_POSSIBLE_NAMES tclreadline-2.1.0
    tclreadline-2.3.2 tclreadline-2.3.6 tclreadline-2.3.7 tclreadline-2.3.8
    )
find_library(TCL_READLINE_LIBRARY
  NAMES tclreadline ${TCL_READLINE_POSSIBLE_NAMES}
  PATHS ${TCL_LIB_PATHS}
  )
if (TCL_READLINE_LIBRARY)
  message(STATUS "TCL readline library: ${TCL_READLINE_LIBRARY}")
endif()

find_path(TCL_READLINE_H tclreadline.h)
if (TCL_READLINE_H)
  message(STATUS "TCL readline header: ${TCL_READLINE_H}")
endif()

option(BUILD_TCLX "Build with tclX included" ON)
if (BUILD_TCLX AND TCLX_LIBRARY)
  message(STATUS "TclX library: ${TCLX_LIBRARY}")
endif()
if (BUILD_TCLX AND TCLX_H)
  message(STATUS "TclX header: ${TCLX_H}")
endif()

find_package(SWIG 3.0 REQUIRED)
include(UseSWIG)

find_package(Boost REQUIRED)
find_package(Python3 COMPONENTS Development REQUIRED)
option(BUILD_PYTHON "Build the Python3 interface" ON)

################################################################

swig_lib(NAME      openroad_swig
         NAMESPACE ord
         I_FILE    OpenRoad.i
         SCRIPTS   ${OPENSTA_HOME}/tcl/Util.tcl
                   OpenRoad.tcl
                   ${ICEWALL_HOME}/src/ICeWall.tcl
                   ${OPENDB_HOME}/src/db/OpenDB.tcl
)

target_link_libraries(openroad_swig
  PRIVATE
    dbSta
    opendb
    OpenSTA
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

find_package(spdlog REQUIRED)
message(STATUS "spdlog: ${spdlog_VERSION}")

find_package(Threads REQUIRED)
set(THREADS_PREFER_PTHREAD_FLAG ON)

################################################################

# Build flow tools
add_subdirectory(ifp)
add_subdirectory(OpenDB)
add_subdirectory(sta)
add_subdirectory(dbSta)
add_subdirectory(rsz)
add_subdirectory(stt)
add_subdirectory(replace)
add_subdirectory(dpl)
add_subdirectory(fin)
add_subdirectory(ppl)
add_subdirectory(TritonCTS)
add_subdirectory(grt)
add_subdirectory(tap)
add_subdirectory(mpl)
add_subdirectory(rcx)
add_subdirectory(psm)
add_subdirectory(PartitionMgr)
add_subdirectory(ant)
add_subdirectory(gui)
add_subdirectory(TritonRoute)
add_subdirectory(utl)
add_subdirectory(pdr)
add_subdirectory(pdngen)

################################################################

add_executable(openroad
  ${OPENROAD_SOURCE}
)

target_compile_options(openroad
  PRIVATE
    -Wall -Wextra -pedantic -Wcast-qual -Wredundant-decls -Wformat-security
    $<$<CXX_COMPILER_ID:Clang>:-Wno-gnu-zero-variadic-macro-arguments>
    $<$<CXX_COMPILER_ID:AppleClang>:-Wno-gnu-zero-variadic-macro-arguments>
)

# Disable compiler specific extensions like gnu++11.
set_target_properties(openroad PROPERTIES CXX_EXTENSIONS OFF)
target_compile_features(openroad PUBLIC cxx_std_17)

target_include_directories(openroad
  PUBLIC
    ../include
)

target_link_libraries(openroad
  ifp
  openroad_swig
  replace
  dpl
  fin
  rsz
  ppl
  stt
  dbSta
  OpenSTA
  opendb
  opendbtcl
  rcx
  TritonCTS
  grt
  tap
  gui
  TritonRoute
  mpl
  psm
  ant
  PartitionMgr
  utl
  pdn
  pdr
  ${TCL_LIBRARY}
  ${CMAKE_THREAD_LIBS_INIT}
)

# tclReadline
if (TCL_READLINE_LIBRARY AND TCL_READLINE_H)
  target_compile_definitions(openroad PRIVATE ENABLE_READLINE)
  target_link_libraries(openroad ${TCL_READLINE_LIBRARY})
  target_include_directories(openroad PRIVATE ${TCL_READLINE_H})
  message(STATUS "TCL readline enabled")
else()
  message(STATUS "TCL readline disabled")
endif()

if (BUILD_TCLX AND TCLX_LIBRARY AND TCLX_H)
  target_compile_definitions(openroad PRIVATE ENABLE_TCLX)
  target_link_libraries(openroad ${TCLX_LIBRARY})
  target_include_directories(openroad PRIVATE ${TCLX_H})
  message(STATUS "Tcl Extended enabled")
else()
  message(STATUS "Tcl Extended disabled")
endif()

if (ZLIB_FOUND)
  target_link_libraries(openroad ${ZLIB_LIBRARIES})
endif()

# Optional CUDD library for OpenSTA
if (CUDD_LIB)
  target_link_libraries(openroad ${CUDD_LIB})
endif()

if (Python3_FOUND AND BUILD_PYTHON)
  message(STATUS "Python3 enabled")
  target_compile_definitions(openroad PRIVATE ENABLE_PYTHON3)

  swig_lib(NAME      openroad_swig_py
		   NAMESPACE ord
		   LANGUAGE  python
		   I_FILE    OpenRoad-py.i
		   SCRIPTS   ${CMAKE_CURRENT_BINARY_DIR}/openroad_swig_py.py
  )

  target_link_libraries(openroad_swig_py
    PRIVATE
      dbSta
      opendb
      OpenSTA
  )
  target_link_libraries(openroad
    openroad_swig_py
    opendbpy
  )
else()
  message(STATUS "Python3 disabled")
endif()

# Ancient versions of glibc found on CentOS required librt.
# OSX has an empty version of librt that causes a link error,
# so this is protected from linking there.
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
    AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
  find_library(LIBRT rt)
  target_link_libraries(openroad ${LIBRT})
endif()

################################################################
# Install
# cmake .. -DCMAKE_INSTALL_PREFIX=<prefix_path>

# executable
install(TARGETS openroad DESTINATION bin)

################################################################

add_custom_target(openroad_tags etags -o TAGS
  ${OPENROAD_SOURCE}
  ${OPENROAD_HOME}/include/ord/*.hh
  ${OPENROAD_HOME}/src/dbSta/src/*.hh
  ${DBSTA_HOME}/include/db_sta/*.hh
  ${OPENROAD_HOME}/src/dbSta/src/*.cc
  ${RESIZER_HOME}/src/*.hh
  ${RESIZER_HOME}/src/*.cc
  WORKING_DIRECTORY ${OPENROAD_HOME}/src
  DEPENDS ${OPENROAD_SOURCE} ${OPENROAD_HEADERS} ${OPENROAD_TCL_FILES}
)
