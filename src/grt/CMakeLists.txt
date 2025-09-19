# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2025, The OpenROAD Authors

include("openroad")

project(grt)

find_package(OpenMP REQUIRED)

add_subdirectory(src/fastroute)
add_subdirectory(src/cugr)

swig_lib(NAME      grt
         NAMESPACE grt
         I_FILE    src/GlobalRouter.i
         SCRIPTS   src/GlobalRouter.tcl
)

add_library(grt_lib
  src/Grid.cpp
  src/Net.cpp
  src/Pin.cpp
  src/RepairAntennas.cpp
  src/Rudy.cpp
  src/GlobalRouter.cpp
)

target_include_directories(grt_lib
  PUBLIC
    include
  PRIVATE
    src
)

target_link_libraries(grt_lib
    utl_lib
    FastRoute4.1
    CUGR
    ant_lib
    dpl_lib
    dbSta_lib
    stt_lib
    rsz_lib
    OpenSTA
    Boost::boost
    OpenMP::OpenMP_CXX
)

target_sources(grt
  PRIVATE
    src/MakeGlobalRouter.cpp
    src/heatMap.cpp
    src/heatMapRudy.cpp
    src/GrouteRenderer.cpp
    src/fastroute/src/FastRouteRenderer.cpp
)

target_include_directories(grt
  PUBLIC
    include
  PRIVATE
    src
)

target_link_libraries(grt
    grt_lib
    dbSta
    gui
    stt
    rsz
)

if (Python3_FOUND AND BUILD_PYTHON)
  swig_lib(NAME          grt_py
           NAMESPACE     grt
           LANGUAGE      python
           I_FILE        src/GlobalRouter-py.i
           SWIG_INCLUDES ${PROJECT_SOURCE_DIR}/include/grt
           SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/grt_py.py
  )

  target_include_directories(grt_py
    PUBLIC
      include
  )

  target_link_libraries(grt_py
    PUBLIC
      utl_lib
      FastRoute4.1
      CUGR
      drt_lib
      ant_lib
      dpl_lib
      dbSta
      gui
      stt
      OpenSTA
      Boost::boost
  )

endif()

messages(
  TARGET grt
)

add_subdirectory(test)
