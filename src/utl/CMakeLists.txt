# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021-2025, The OpenROAD Authors

include("openroad")

find_package(Boost CONFIG 1.78 REQUIRED COMPONENTS iostreams)

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

add_library(utl_lib
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
  src/decode.cpp
  src/prometheus/metrics_server.cpp
  src/histogram.cpp
  src/unionFind.cpp
)

target_include_directories(utl_lib
  PUBLIC
    include
  PRIVATE
    src
    ${Boost_INCLUDE_DIRS}
    ${TCL_INCLUDE_PATH}
)

target_link_libraries(utl_lib
  PUBLIC
    absl::synchronization
    spdlog::spdlog
    ${TCL_LIBRARY}
    ${Boost_LIBRARIES}
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

