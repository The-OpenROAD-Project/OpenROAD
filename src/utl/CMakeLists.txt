# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021-2025, The OpenROAD Authors

include("openroad")

swig_lib(NAME      utl
         NAMESPACE utl
         I_FILE    src/Logger.i
         SCRIPTS   src/Utl.tcl
         SWIG_INCLUDES ${PROJECT_SOURCE_DIR}/src
)

if (Python3_FOUND AND BUILD_PYTHON)
  swig_lib(NAME          utl_py
           NAMESPACE     utl
           LANGUAGE      python
           I_FILE        src/Logger-py.i
           SWIG_INCLUDES ${PROJECT_SOURCE_DIR}/include/utl
                         ${PROJECT_SOURCE_DIR}/src
           SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/utl_py.py
  )

  target_include_directories(utl_py
    PUBLIC
      include
    PRIVATE
      src
  )
  
  target_link_libraries(utl_py
    PUBLIC
      spdlog::spdlog
  )
endif()

#
# Isolate TCL dependency to decode
#
add_library(utl_decode
  src/decode.cpp
)

target_include_directories(utl_decode
  PUBLIC
    include
  PRIVATE
    src
    ${TCL_INCLUDE_PATH}
)

target_link_libraries(utl_decode
  PUBLIC
    spdlog::spdlog
  PRIVATE
    ${TCL_LIBRARY}
)

#
# Avoid TCL in the rest of utl
#
add_library(utl_base
  src/CallBackHandler.cpp
  src/CallBack.cpp
  src/Metrics.cpp
  src/CFileUtils.cpp
  src/ScopedTemporaryFile.cpp
  src/SuppressStdout.cpp
  src/Logger.cpp
  src/Progress.cpp
  src/CommandLineProgress.cpp
  src/timer.cpp
  src/mem_stats.cpp
  src/prometheus/metrics_server.cpp
  src/histogram.cpp
)

target_include_directories(utl_base
  PUBLIC
    include
  PRIVATE
    src
    ${Boost_INCLUDE_DIRS}
    absl::stacktrace
    absl::symbolize
)

target_link_libraries(utl_base
  PUBLIC
    spdlog::spdlog
    ${Boost_LIBRARIES}
    absl::stacktrace
    absl::symbolize
)

add_library(utl_lib INTERFACE)

target_link_libraries(utl_lib
  INTERFACE
    utl_base
    utl_decode
)

target_sources(utl
  PRIVATE
    src/LoggerCommon.cpp
    src/MakeLogger.cpp
)
  
target_include_directories(utl
  PUBLIC
    include
  PRIVATE
    src
    ${Boost_INCLUDE_DIRS}
)

target_compile_definitions(utl
  PUBLIC
    FMT_DEPRECATED_OSTREAM=1
)

target_link_libraries(utl
  PUBLIC
    utl_lib
    utl_decode
)

messages(
  TARGET utl
)

############################################################
# Unit testing
############################################################\
if(ENABLE_TESTS)
  add_subdirectory(test)
endif()
