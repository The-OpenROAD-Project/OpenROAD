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

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.13)
# Use standard target names
cmake_policy(SET CMP0078 NEW)
endif()

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.14)
# Allows SWIG_MODULE_NAME to be set
cmake_policy(SET CMP0086 NEW)
endif()

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.12)
# Allows <PackageName>_ROOT to be used
cmake_policy(SET CMP0074 NEW)
endif()

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.10)
# Allow AUTOUIC on generated source
cmake_policy(SET CMP0071 NEW)
endif()

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.13)
# option() behavior
cmake_policy(SET CMP0077 NEW)
endif()

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.10)
# Let AUTOMOC and AUTOUIC process GENERATED files.
cmake_policy(SET CMP0071 NEW)
endif()

project(OpenROAD VERSION 1
  LANGUAGES CXX
)

set(OPENROAD_HOME ${PROJECT_SOURCE_DIR})

set(CMAKE_VERBOSE_MAKEFILE ON)
# Default c++ standard used unless otherwise specified in target_compile_features.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Disable compiler specific extensions like gnu++11.
set(CMAKE_CXX_EXTENSIONS OFF)

set(OPENROAD_VERSION ${PROJECT_VERSION})
message(STATUS "OpenROAD version: ${OPENROAD_VERSION}")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Get version string in OPENROAD_GIT_DESCRIBE
include(GetGitRevisionDescription)
git_describe(OPENROAD_GIT_DESCRIBE)
string(FIND ${OPENROAD_GIT_DESCRIBE} "NOTFOUND" GIT_DESCRIBE_NOTFOUND)
if(${GIT_DESCRIBE_NOTFOUND} GREATER -1)
  message(WARNING "OpenROAD git describe failed, using sha1 instead")
  get_git_head_revision(GIT_REFSPEC OPENROAD_GIT_DESCRIBE)
endif()
message(STATUS "OpenROAD git describe: ${OPENROAD_GIT_DESCRIBE}")

# Default to bulding optimnized/release executable.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RELEASE)
endif()

message(STATUS "System name: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")

# configure a header file to pass some of the CMake settings
configure_file(
  ${OPENROAD_HOME}/include/ord/Version.hh.cmake
  ${OPENROAD_HOME}/include/ord/Version.hh
  )

################################################################

# Ask CMake to output a compile_commands.json file for use with things like clang-tidy
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

add_subdirectory(src)
