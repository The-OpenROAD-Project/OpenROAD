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

cmake_minimum_required (VERSION 3.9)

project(OpenROAD VERSION 1.0.0)

set(OPENROAD_HOME ${PROJECT_SOURCE_DIR})

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Disable compiler specific extensions like gnu++11.
set(CMAKE_CXX_EXTENSIONS OFF)

set(OPENROAD_VERSION ${PROJECT_VERSION})
message(STATUS "OpenROAD version: ${OPENROAD_VERSION}")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC OPENROAD_GIT_SHA1)
message(STATUS "OpenROAD git sha: ${OPENROAD_GIT_SHA1}")

# Default to bulding optimnized/release executable.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RELEASE)
endif()

# Compiler specific options.
# Note -Wno-deprecated-register is to suppress bison errors.
if (CMAKE_CXX_COMPILER_ID MATCHES AppleClang|Clang)
  set(STA_COMPILE_OPTIONS -Wall -Wextra -pedantic -Wcast-qual -Wredundant-decls -Wformat-security -Wno-deprecated-register)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(STA_COMPILE_OPTIONS -Wall -Wextra -pedantic -Wcast-qual -Wredundant-decls -Wformat-security)
endif()

message(STATUS "System name: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Build CXX_FLAGS: ${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}}")
message(STATUS "CXX_FLAGS: ${COMPILE_OPTIONS}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")

# configure a header file to pass some of the CMake settings
configure_file(
  ${OPENROAD_HOME}/include/openroad/Version.hh.cmake
  ${OPENROAD_HOME}/include/openroad/Version.hh
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

include(FindSWIG)

################################################################

add_subdirectory(src)
