###############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2019, The Regents of the University of California
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

include("openroad")

project(grt)

find_package(OpenMP REQUIRED)

add_subdirectory(src/fastroute)

swig_lib(NAME      grt
         NAMESPACE grt
         I_FILE    src/GlobalRouter.i
         SCRIPTS   src/GlobalRouter.tcl
)

add_library(grt_lib
    src/Grid.cpp
    src/Net.cpp
    src/Pin.cpp
    src/MakeWireParasitics.cpp
    src/RepairAntennas.cpp
    src/Rudy.cpp
    src/GlobalRouter.cpp
)

target_sources(grt
  PRIVATE
    src/MakeGlobalRouter.cpp
    src/heatMap.cpp
    src/heatMapRudy.cpp
    src/GrouteRenderer.cpp
    src/fastroute/src/FastRouteRenderer.cpp
)

target_include_directories(grt_lib
  PUBLIC
    include
  PRIVATE
    src
)

target_include_directories(grt
  PUBLIC
    include
  PRIVATE
    src
)

target_link_libraries(grt_lib
    utl_lib
    FastRoute4.1
    ant_lib
    dpl_lib
    dbSta_lib
    stt_lib
    rsz_lib
    OpenSTA
    Boost::boost
    OpenMP::OpenMP_CXX
)

target_link_libraries(grt
    grt_lib
    utl
    FastRoute4.1
    ant_lib
    dpl_lib
    dbSta
    gui
    stt
    rsz
    OpenSTA
    Boost::boost
    OpenMP::OpenMP_CXX
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
      utl
      FastRoute4.1
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