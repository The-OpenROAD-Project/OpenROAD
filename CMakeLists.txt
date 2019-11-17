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

add_subdirectory(src)
