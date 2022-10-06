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

if(USE_SYSTEM_OPENSTA)
  if(NOT OPENSTA_HOME)
    message(FATAL_ERROR "Please set external OPENSTA_HOME path.")
  endif()
else()
  set(OPENSTA_HOME ${PROJECT_SOURCE_DIR}/src/sta)
endif()

set(ODB_HOME ${PROJECT_SOURCE_DIR}/src/odb)
set(DBSTA_HOME ${PROJECT_SOURCE_DIR}/src/dbSta)
set(RESIZER_HOME ${PROJECT_SOURCE_DIR}/src/rsz)
set(PAD_HOME ${OPENROAD_HOME}/src/pad)

set(OPENROAD_SOURCE
  Design.cc
  Tech.cc
  OpenRoad.cc
  Main.cc
  )

if(USE_SYSTEM_BOOST)
  set(Boost_USE_STATIC_LIBS OFF)
else()
  set(Boost_USE_STATIC_LIBS ON)
endif()

if(USE_SYSTEM_OPENSTA)
  if(NOT OPENSTA_LIBRARY)
    find_library(OPENSTA_LIBRARY NAMES OpenSTA
                 PATHS
                 /usr/lib
                 /usr/lib64
                 /usr/local/lib
                 /usr/local/lib64)
    if(OPENSTA_LIBRARY)
      message(STATUS "Found OpenSTA: ${OPENSTA_LIBRARY}")
    else()
      message(FATAL_ERROR "OpenSTA not found, set the OPENSTA_LIBRARY manually.")
    endif()
  endif()
  if(NOT OPENSTA_INCLUDE_DIR)
    set(OPENSTA_INCLUDE_DIR "/usr/include/sta")
  endif()
  include_directories(${OPENSTA_INCLUDE_DIR})
  message(STATUS "Using OPENSTA_INCLUDE_DIR=${OPENSTA_INCLUDE_DIR}")
else()
  # local build of opensta
  set(OPENSTA_LIBRARY "sta_swig")
endif()

if(USE_SYSTEM_ABC)
  if(NOT ABC_LIBRARY)
    find_library(ABC_LIBRARY NAMES abc
                 PATHS
                 /usr/lib
                 /usr/lib64
                 /usr/local/lib
                 /usr/local/lib64)
    if(ABC_LIBRARY)
      message(STATUS "Found ABC: ${ABC_LIBRARY}")
      # make sure link dl symbols
      list(APPEND ABC_LIBRARY "dl")
    else()
      message(FATAL_ERROR "ABC not found, set the ABC_LIBRARY manually.")
    endif()
  endif()
  if(NOT ABC_INCLUDE_DIR)
    set(ABC_INCLUDE_DIR "/usr/include/abc")
  endif()
  add_definitions(-DABC_USE_STDINT_H=1)
  include_directories(${ABC_INCLUDE_DIR})
  message(STATUS "Using ABC_INCLUDE_DIR=${ABC_INCLUDE_DIR}")
else()
  # local build of abc
  set(ABC_LIBRARY "libabc")
endif()

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
message(STATUS "boost: ${Boost_VERSION}")
find_package(Python3 COMPONENTS Development REQUIRED)
option(BUILD_PYTHON "Build the Python3 interface" ON)

option(ALLOW_WARNINGS "Flag to allow compilation with compiler warnings: on by default" ON)
if (NOT ALLOW_WARNINGS)
  add_compile_options(
    $<$<CXX_COMPILER_ID:GNU>:-Werror>
  )
endif()

add_compile_options(
  -Wall
  -Wredundant-decls
  -Wformat-security
  -Wno-sign-compare
  -Wp,-D_GLIBCXX_ASSERTIONS
  $<$<CXX_COMPILER_ID:Clang>:-Wno-gnu-zero-variadic-macro-arguments>
  $<$<CXX_COMPILER_ID:AppleClang>:-Wno-gnu-zero-variadic-macro-arguments>
)

################################################################

swig_lib(NAME      openroad_swig
         NAMESPACE ord
         I_FILE    OpenRoad.i
         SCRIPTS   ${OPENSTA_HOME}/tcl/Util.tcl
                   OpenRoad.tcl
                   Metrics.tcl
                   ${PAD_HOME}/src/pad.tcl
                   ${ODB_HOME}/src/db/odb.tcl
)

target_link_libraries(openroad_swig
  PRIVATE
    dbSta
    odb
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
if (${SPDLOG_FMT_EXTERNAL})
  message(WARNING "spdlog: SPDLOG_FMT_EXTERNAL=${SPDLOG_FMT_EXTERNAL}")
endif()

find_package(Threads REQUIRED)
set(THREADS_PREFER_PTHREAD_FLAG ON)

################################################################

# Build flow tools
add_subdirectory(ifp)
add_subdirectory(odb)
if (NOT USE_SYSTEM_OPENSTA)
  add_subdirectory(sta)
endif()
add_subdirectory(dbSta)
add_subdirectory(rsz)
add_subdirectory(stt)
add_subdirectory(gpl)
add_subdirectory(dpl)
add_subdirectory(dpo)
add_subdirectory(fin)
add_subdirectory(ppl)
add_subdirectory(rmp)
add_subdirectory(cts)
add_subdirectory(grt)
add_subdirectory(tap)
add_subdirectory(mpl)
add_subdirectory(mpl2)
add_subdirectory(rcx)
add_subdirectory(psm)
add_subdirectory(par)
add_subdirectory(ant)
add_subdirectory(gui)
add_subdirectory(drt)
add_subdirectory(utl)
add_subdirectory(dst)
add_subdirectory(pdn)

################################################################

add_executable(openroad
  ${OPENROAD_SOURCE}
)

target_compile_options(openroad
  PRIVATE
    -Wextra -pedantic -Wcast-qual
)

# Needed for boost stacktrace
if(APPLE)
  target_compile_definitions(openroad PUBLIC "-D_GNU_SOURCE")
endif()


set_target_properties(openroad PROPERTIES
  # Disable compiler specific extensions like gnu++11.
  CXX_EXTENSIONS OFF
  # Export symbols for stack trace printing
  ENABLE_EXPORTS ON
)
target_compile_features(openroad PUBLIC cxx_std_17)

target_include_directories(openroad
  PUBLIC
    ../include
)

target_link_libraries(openroad
  ifp
  openroad_swig
  gpl
  dpl
  dpo
  fin
  rsz
  ppl
  stt
  dbSta
  OpenSTA
  odb
  odbtcl
  rcx
  rmp
  cts
  grt
  tap
  gui
  drt
  dst
  mpl
  mpl2
  psm
  ant
  par
  utl
  pdn
  ${ABC_LIBRARY}
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
      odb
      OpenSTA
  )
  target_link_libraries(openroad
    openroad_swig_py
    odbpy
    ifp_py
    utl_py
  )
else()
  message(STATUS "Python3 disabled")
endif()

messages(
  TARGET openroad
  SOURCE_DIR .
  OUTPUT_DIR ..
  LOCAL
)

# pad isn't a proper module with its own CMakeLists.txt so it
# is handled here (just like the tcl encoding).
messages(
  TARGET openroad
  OUTPUT_DIR pad
)

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
