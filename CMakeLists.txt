# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2025, The OpenROAD Authors

cmake_minimum_required (VERSION 3.16)

# Use standard target names
cmake_policy(SET CMP0078 NEW)

# Allows SWIG_MODULE_NAME to be set
cmake_policy(SET CMP0086 NEW)

# Allows <PackageName>_ROOT to be used
cmake_policy(SET CMP0074 NEW)

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.27")
  # Disallows <PACKAGENAME>_ROOT in find_package.
  # TODO: resolve absl/or-tools issue and switch to NEW
  cmake_policy(SET CMP0144 OLD)
endif()

# Allow AUTOUIC on generated source
cmake_policy(SET CMP0071 NEW)

# option() behavior
cmake_policy(SET CMP0077 NEW)

# Let AUTOMOC and AUTOUIC process GENERATED files.
cmake_policy(SET CMP0071 NEW)

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
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Disable compiler specific extensions like gnu++11.
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Get version string in OPENROAD_VERSION
if(NOT OPENROAD_VERSION)
  include(GetGitRevisionDescription)
  git_describe(OPENROAD_VERSION --tags --match "[0-9][0-9]Q[0-9]")
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

# Detect OS distribution and version from /etc/os-release
# LTO is known to cause build errors on Debian 11 and Rocky Linux 8
set(LTO_UNSUPPORTED_OS FALSE)
if(EXISTS "/etc/os-release")
  file(STRINGS "/etc/os-release" OS_RELEASE_CONTENTS)
  set(OS_ID "")
  set(OS_VERSION_ID "")
  foreach(line ${OS_RELEASE_CONTENTS})
    if(line MATCHES "^ID=(.*)$")
      string(REGEX REPLACE "^ID=\"?([^\"]*)\"?$" "\\1" OS_ID "${line}")
    endif()
    if(line MATCHES "^VERSION_ID=(.*)$")
      string(REGEX REPLACE "^VERSION_ID=\"?([^\"]*)\"?$" "\\1" OS_VERSION_ID "${line}")
    endif()
  endforeach()

  # Check for unsupported OS for LTO
  if((OS_ID STREQUAL "debian" AND OS_VERSION_ID STREQUAL "11") OR
     (OS_ID STREQUAL "rocky" AND OS_VERSION_ID MATCHES "^8"))
    set(LTO_UNSUPPORTED_OS TRUE)
    message(STATUS "Detected unsupported OS (${OS_ID} ${OS_VERSION_ID}): LTO will be disabled due to known build issues")
  endif()
endif()

option(LINK_TIME_OPTIMIZATION "Flag to control link time optimization: on by default" ON)
set(LTO_STATUS "disabled")
if (LINK_TIME_OPTIMIZATION AND NOT LTO_UNSUPPORTED_OS)
  include(CheckIPOSupported)
  check_ipo_supported(RESULT lto_supported OUTPUT lto_error)
  if(lto_supported)
    # LTO is enabled globally for Release build
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO FALSE)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG FALSE)

    string(TOUPPER "${CMAKE_BUILD_TYPE}" UPPER_BUILD_TYPE)
    if(UPPER_BUILD_TYPE STREQUAL "RELEASE")
      set(LTO_STATUS "enabled")
    endif()
  else()
    set(LTO_STATUS "not supported: ${lto_error}")
  endif()
endif()
message(STATUS "LTO/IPO is ${LTO_STATUS}")

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

# Not currently used but may become useful again with Kokkos
target_compile_definitions(openroad PRIVATE GPU)

if(BUILD_PYTHON)
  target_compile_definitions(openroad PRIVATE BUILD_PYTHON=1)
else()
  target_compile_definitions(openroad PRIVATE BUILD_PYTHON=0)
endif()

if(BUILD_GUI)
  target_compile_definitions(openroad PRIVATE BUILD_GUI=1)
else()
  target_compile_definitions(openroad PRIVATE BUILD_GUI=0)
endif()

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

####################################################################
# Add a configuration for ReleaseWithAsserts
set(CMAKE_CXX_FLAGS_RELEASEWITHASSERTS "${CMAKE_CXX_FLAGS_RELEASE} -UNDEBUG" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELEASEWITHASSERTS "${CMAKE_C_FLAGS_RELEASE} -UNDEBUG" CACHE STRING "" FORCE)
