# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2025, The OpenROAD Authors

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/src/cmake")

# The location used by etc/DependencyInstaller.sh
list(APPEND CMAKE_PREFIX_PATH
  "/opt/or-tools/lib64/cmake"
  "/opt/or-tools/lib/cmake"
)

# Link to where ortools was found
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

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
set(GUI_HOME ${PROJECT_SOURCE_DIR}/src/gui)

set(OPENROAD_SOURCE
  Design.cc
  Timing.cc
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
  add_library(OpenSTA STATIC IMPORTED)
  set_target_properties(OpenSTA PROPERTIES
    IMPORTED_LOCATION "${OPENSTA_LIBRARY}"
  )
  if(CUDD_LIB)
    set_target_properties(OpenSTA PROPERTIES
      INTERFACE_LINK_LIBRARIES "${CUDD_LIB}"
    )
  endif()
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

find_package(SWIG 4.0 REQUIRED)
if (SWIG_VERSION VERSION_GREATER_EQUAL "4.1.0")
  message(STATUS "Using SWIG >= ${SWIG_VERSION} -flatstaticmethod flag for python")
endif()
include(UseSWIG)

find_package(Boost CONFIG REQUIRED)
message(STATUS "boost: ${Boost_VERSION}")

find_package(absl REQUIRED)

if (ENABLE_TESTS)
  find_package(GTest REQUIRED)
  message(STATUS "GTest: ${GTest_VERSION}")
endif()

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
  -Wmissing-field-initializers
  -Wredundant-decls
  -Wformat-security
  -Wno-sign-compare
  -Wp,-D_GLIBCXX_ASSERTIONS
  $<$<CXX_COMPILER_ID:Clang>:-Wno-gnu-zero-variadic-macro-arguments>
  # Needed for floating point stability in FFT (fft_test will check this).
  # See also https://kristerw.github.io/2021/11/09/fp-contract/
  -ffp-contract=off
  # Apple clang 14.0.0 deprecates sprintf, which generates 900 warnings.
  $<$<CXX_COMPILER_ID:AppleClang>:-Wno-deprecated-declarations>
  $<$<CXX_COMPILER_ID:AppleClang>:-Wmismatched-tags>
  $<$<BOOL:${ASAN}>:-fsanitize=address>
  $<$<BOOL:${ASAN}>:-fno-omit-frame-pointer>
  $<$<BOOL:${ASAN}>:-g>
  $<$<BOOL:${TSAN}>:-fsanitize=thread>
  $<$<BOOL:${TSAN}>:-g>
  $<$<BOOL:${UBSAN}>:-fsanitize=undefined>
  $<$<BOOL:${UBSAN}>:-g>
  $<$<BOOL:${UBSAN}>:-fno-omit-frame-pointer>
)

if (ASAN)
  add_link_options(-fsanitize=address)
  message(STATUS "Address Sanitizer is enabled")
endif()

if (TSAN)
  add_link_options(-fsanitize=thread)
  message(STATUS "Thread Sanitizer is enabled")
endif()

if (UBSAN)
  add_link_options(-fsanitize=undefined)
  message(STATUS "Undefined Behavior Sanitizer is enabled")
endif()

################################################################

swig_lib(NAME      ord
         NAMESPACE ord
         I_FILE    OpenRoad.i
         SCRIPTS   ${OPENSTA_HOME}/tcl/Util.tcl
                   OpenRoad.tcl
                   Metrics.tcl
)

target_link_libraries(ord
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
add_subdirectory(pad)
add_subdirectory(odb)
if (NOT USE_SYSTEM_OPENSTA)
  if (TCL_READLINE_LIBRARY AND TCL_READLINE_H)
    # Pass along tcl readline enablement to OpenSTA build
    set(USE_TCL_READLINE ON)
    set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
  endif()
  add_subdirectory(sta)
endif()
add_subdirectory(dbSta)
add_subdirectory(rsz)
add_subdirectory(tst)
add_subdirectory(stt)
add_subdirectory(gpl)
add_subdirectory(dpl)
add_subdirectory(exa)
add_subdirectory(fin)
add_subdirectory(ram)
add_subdirectory(ppl)
add_subdirectory(rmp)
add_subdirectory(cgt)
add_subdirectory(cts)
add_subdirectory(cut)
add_subdirectory(grt)
add_subdirectory(tap)
add_subdirectory(rcx)
add_subdirectory(psm)
add_subdirectory(ant)
add_subdirectory(gui)
add_subdirectory(drt)
add_subdirectory(upf)
add_subdirectory(utl)
add_subdirectory(dst)
add_subdirectory(pdn)
add_subdirectory(dft)
add_subdirectory(mpl)
add_subdirectory(par)
add_subdirectory(est)

################################################################

add_executable(openroad
  ${OPENROAD_SOURCE}
)

set_target_properties(openroad PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
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
target_compile_features(openroad PUBLIC cxx_std_20)

target_include_directories(openroad
  PUBLIC
    ../include
)

target_link_libraries(openroad
  ifp
  pad
  ord
  gpl
  dpl
  exa
  fin
  ram
  rsz
  ppl
  stt
  dbSta
  OpenSTA
  odb
  odbtcl
  rcx
  rmp
  cgt
  cts
  cut
  grt
  tap
  gui
  drt
  dst
  psm
  ant
  upf
  utl
  pdn
  dft
  mpl
  par
  est
  absl::synchronization
  ${ABC_LIBRARY}
  ${TCL_LIBRARY}
  ${CMAKE_THREAD_LIBS_INIT}
  ${Boost_LIBRARIES}
)

target_compile_definitions(openroad PRIVATE BUILD_TYPE="${CMAKE_BUILD_TYPE}")

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

  swig_lib(NAME      ord_py
           NAMESPACE ord
           LANGUAGE  python
           I_FILE    OpenRoad-py.i
           SCRIPTS   ${CMAKE_CURRENT_BINARY_DIR}/ord_py.py
  )

  target_link_libraries(ord_py
    PRIVATE
      dbSta
      odb
      OpenSTA
      ifp
  )
  target_link_libraries(openroad
    ord_py
    odb_py
    ifp_py
    utl_py
    ant_py
    grt_py
    gpl_py
    dpl_py
    exa_py
    ppl_py
    tap_py
    cgt_py
    cts_py
    drt_py
    fin_py
#    ram_py
    rcx_py
    rmp_py
    stt_py
    psm_py
    pdn_py
    dft_py
    par_py
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
