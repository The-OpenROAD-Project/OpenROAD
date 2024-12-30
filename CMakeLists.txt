###############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2019, Parallax Software, Inc.
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of source code must retain the above copyright notice, this
##   list of conditions and the following disclaimer.
##
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and#or other materials provided with the distribution.
##
## * Neither the name of the copyright holder nor the names of its
##   contributors may be used to endorse or promote products derived from
##   this software without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
## LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
## CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
## SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
##
###############################################################################

cmake_minimum_required (VERSION 3.16)

# Use standard target names
cmake_policy(SET CMP0078 NEW)

# Allows SWIG_MODULE_NAME to be set
cmake_policy(SET CMP0086 NEW)

# Allows <PackageName>_ROOT to be used
cmake_policy(SET CMP0074 NEW)

# Allow AUTOUIC on generated source
cmake_policy(SET CMP0071 NEW)

# option() behavior
cmake_policy(SET CMP0077 NEW)

# Let AUTOMOC and AUTOUIC process GENERATED files.
cmake_policy(SET CMP0071 NEW)

# Interfers with Qt so off by default.
option(LINK_TIME_OPTIMIZATION "Flag to control link time optimization: off by default" OFF)

if (LINK_TIME_OPTIMIZATION)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# Allow to use external shared boost libraries
option(USE_SYSTEM_BOOST "Use system shared Boost libraries" OFF)

# Allow to use external shared opensta libraries
option(USE_SYSTEM_OPENSTA "Use system shared OpenSTA library" OFF)

# Allow to use external shared abc libraries
option(USE_SYSTEM_ABC "Use system shared ABC library" OFF)

# Allow disabling tests
option(ENABLE_TESTS "Enable OpenROAD tests" ON)

# Allow enabling address sanitizer
option(ASAN "Enable Address Sanitizer" OFF)

# Allow enabling address sanitizer
#  On Ubuntu22.04 you need to use "sudo sysctl vm.mmap_rnd_bits=28" to
#  resolve a known fatal
option(TSAN "Enable Thread Sanitizer" OFF)

# Allow enabling address sanitizer
option(UBSAN "Enable Undefined Behavior Sanitizer" OFF)

project(OpenROAD VERSION 1
  LANGUAGES CXX
)

set(OPENROAD_HOME ${PROJECT_SOURCE_DIR})
set(OPENROAD_SHARE ${CMAKE_INSTALL_PREFIX}/share/openroad)

# Default c++ standard used unless otherwise specified in target_compile_features.
set(CMAKE_CXX_STANDARD 17 CACHE STRING "the C++ standard to use for this project")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Disable compiler specific extensions like gnu++11.
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Get version string in OPENROAD_VERSION
if(NOT OPENROAD_VERSION)
  include(GetGitRevisionDescription)
  git_describe(OPENROAD_VERSION)
  string(FIND ${OPENROAD_VERSION} "NOTFOUND" GIT_DESCRIBE_NOTFOUND)
  if(${GIT_DESCRIBE_NOTFOUND} GREATER -1)
    message(WARNING "OpenROAD git describe failed, using sha1 instead")
    get_git_head_revision(GIT_REFSPEC OPENROAD_VERSION)
  endif()
endif()

message(STATUS "OpenROAD version: ${OPENROAD_VERSION}")

# Default to bulding optimnized/release executable.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RELEASE)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "8.3.0")
    message(FATAL_ERROR "Insufficient gcc version. Found ${CMAKE_CXX_COMPILER_VERSION}, but require  >= 8.3.0.")
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "7.0.0")
    message(FATAL_ERROR "Insufficient Clang version. Found ${CMAKE_CXX_COMPILER_VERSION}, but require >= 7.0.0.")
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "12.0.0")
    message(FATAL_ERROR "Insufficient AppleClang version. Found ${CMAKE_CXX_COMPILER_VERSION}, but require >= 12.0.0.")
  endif()
else()
  message(WARNING "Compiler ${CMAKE_CXX_COMPILER_ID} is not officially supported.")
endif()

message(STATUS "System name: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "C++ Standard Required: ${CMAKE_CXX_STANDARD_REQUIRED}")
message(STATUS "C++ Extensions: ${CMAKE_CXX_EXTENSIONS}")

# configure a header file to pass some of the CMake settings
configure_file(
  ${OPENROAD_HOME}/include/ord/Version.hh.cmake
  ${OPENROAD_HOME}/include/ord/Version.hh
  )


################################################################

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9.1")
  MESSAGE(STATUS "Older version of GCC detected. Linking against stdc++fs")
  link_libraries(stdc++fs)
endif()

# Ask CMake to output a compile_commands.json file for use with things like clang-tidy
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

add_subdirectory(third-party)

if(ENABLE_TESTS)
  find_program (BASH_PROGRAM bash REQUIRED)

  enable_testing()
  add_custom_target(build_and_test ${CMAKE_CTEST_COMMAND} --parallel --output-on-failure -LE IntegrationTest)
  include(GoogleTest)
endif()

add_subdirectory(src)
add_subdirectory(test)

# After all compiled, look for the respective flags
target_compile_definitions(openroad PRIVATE GPU)
target_compile_definitions(openroad PRIVATE BUILD_PYTHON)
target_compile_definitions(openroad PRIVATE BUILD_GUI)

####################################################################

# Build man pages (Optional)

option(BUILD_MAN "Enable building man pages" OFF)
if(BUILD_MAN)
  message(STATUS "man is enabled")

  # Use the processor_count command to get the number of cores
  include(ProcessorCount)
  ProcessorCount(PROCESSOR_COUNT)
  message(STATUS "Number of processor cores: ${PROCESSOR_COUNT}")

  add_custom_target(
    man_page ALL
    COMMAND make clean && make preprocess && make all -j${PROCESSOR_COUNT}
    WORKING_DIRECTORY ${OPENROAD_HOME}/docs
  )
  
  # Based on ${CMAKE_INSTALL_PREFIX}, we want to go to ${CMAKE_INSTALL_PREFIX}/share/man
  install(DIRECTORY ${OPENROAD_HOME}/docs/cat DESTINATION ${OPENROAD_SHARE}/man)
  install(DIRECTORY ${OPENROAD_HOME}/docs/html DESTINATION ${OPENROAD_SHARE}/man)
endif()
