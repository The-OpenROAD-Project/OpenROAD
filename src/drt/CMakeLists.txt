###############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2021, The Regents of the University of California
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

option(DEBUG_DRT_UNDERFLOW "Check for underflow in drt cost calculations" OFF)

project(drt
  LANGUAGES CXX
)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT error)

find_package(Boost REQUIRED COMPONENTS serialization)
find_package(OpenMP REQUIRED)
find_package(VTune)

swig_lib(NAME      drt
         NAMESPACE drt
         I_FILE    src/TritonRoute.i
         SCRIPTS   src/TritonRoute.tcl
)

set(FLEXROUTE_HOME ${PROJECT_SOURCE_DIR})

target_sources(drt
  PRIVATE
    src/gr/FlexGRCMap.cpp
    src/gr/FlexGR.cpp
    src/gr/FlexGR_end.cpp
    src/gr/FlexGRGridGraph.cpp
    src/gr/FlexGRGridGraph_maze.cpp
    src/gr/FlexGR_init.cpp
    src/gr/FlexGR_maze.cpp
    src/gr/FlexGR_rq.cpp
    src/gr/FlexGR_topo.cpp
    src/dr/FlexDR_conn.cpp
    src/dr/FlexDR_init.cpp
    src/dr/FlexDR.cpp
    src/db/drObj/drNet.cpp
    src/dr/FlexDR_maze.cpp
    src/dr/FlexGridGraph_maze.cpp
    src/dr/FlexGridGraph.cpp
    src/dr/FlexDR_rq.cpp
    src/dr/FlexDR_end.cpp
    src/dr/FlexDR_graphics.cpp
    src/ta/FlexTA_end.cpp
    src/ta/FlexTA_init.cpp
    src/ta/FlexTA_rq.cpp
    src/ta/FlexTA_assign.cpp
    src/ta/FlexTA.cpp
    src/ta/FlexTA_graphics.cpp
    src/global.cpp
    src/gc/FlexGC_end.cpp
    src/gc/FlexGC_rq.cpp
    src/gc/FlexGC.cpp
    src/gc/FlexGC_init.cpp
    src/gc/FlexGC_main.cpp
    src/gc/FlexGC_eol.cpp
    src/gc/FlexGC_inf.cpp
    src/gc/FlexGC_cut.cpp
    src/gc/FlexGC_metspc.cpp
    src/db/drObj/drAccessPattern.cpp
    src/db/drObj/drPin.cpp
    src/db/drObj/drShape.cpp
    src/db/drObj/drVia.cpp
    src/db/infra/frTime_helper.cpp
    src/db/infra/frTime.cpp
    src/db/infra/KDTree.cpp
    src/db/taObj/taShape.cpp
    src/db/obj/frShape.cpp
    src/db/obj/frInst.cpp
    src/db/obj/frVia.cpp
    src/db/obj/frAccess.cpp
    src/db/obj/frRPin.cpp
    src/db/obj/frNode.cpp
    src/db/obj/frInstTerm.cpp
    src/db/obj/frNet.cpp
    src/db/tech/frConstraint.cc
    src/db/obj/frMarker.cpp
    src/db/tech/frLayer.cc
    src/frRegionQuery.cpp
    src/io/io_pin.cpp
    src/io/io.cpp
    src/io/GuideProcessor.cpp
    src/io/io_parser_helper.cpp
    src/pa/FlexPA_init.cpp
    src/pa/FlexPA.cpp
    src/pa/FlexPA_acc_point.cpp
    src/pa/FlexPA_acc_pattern.cpp
    src/pa/FlexPA_row_pattern.cpp
    src/pa/FlexPA_unique.cpp
    src/pa/FlexPA_graphics.cpp
    src/rp/FlexRP_init.cpp
    src/rp/FlexRP.cpp
    src/rp/FlexRP_prep.cpp
    src/distributed/frArchive.cpp
    src/distributed/drUpdate.cpp
    src/distributed/paUpdate.cpp
    src/TritonRoute.cpp
    src/MakeTritonRoute.cpp
    src/frBaseTypes.cpp
    src/DesignCallBack.cpp
)

target_include_directories(drt
  PUBLIC
    include

  PRIVATE
    src
)

target_link_libraries(drt
  PUBLIC
    gui
    odb
    stt
    OpenSTA
    utl
    dst
    dbSta
    Threads::Threads
    OpenMP::OpenMP_CXX
    ${Boost_LIBRARIES}
    ZLIB::ZLIB
)

messages(
  TARGET drt
)

############################################################
# Compiler flags
############################################################
# Todo: add -Wextra and cleanup warnings
target_compile_options(drt
  PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall -pedantic -Wcast-qual -Wredundant-decls -Wformat-security>
    $<$<CXX_COMPILER_ID:AppleClang>:-Wall -pedantic -Wcast-qual -Wredundant-decls -Wformat-security -Wno-gnu-zero-variadic-macro-arguments>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -pedantic -Wcast-qual -Wredundant-decls -Wformat-security -Wno-gnu-zero-variadic-macro-arguments>
  )

############################################################
# Unit testing
############################################################
if(ENABLE_TESTS)
  enable_testing()

  add_executable(trTest
    ${FLEXROUTE_HOME}/test/gcTest.cpp
    ${FLEXROUTE_HOME}/test/fixture.cpp
    ${FLEXROUTE_HOME}/test/stubs.cpp
    ${OPENROAD_HOME}/src/gui/src/stub.cpp
  )

  target_include_directories(trTest
    PRIVATE
    ${FLEXROUTE_HOME}/src
    ${OPENROAD_HOME}/include
  )

  target_link_libraries(trTest
    drt
    odb
  )

  # Use the shared library if found.  We need to pass this info to
  # the code to select the corresponding include.  Using the shared
  # library speeds up compilation.
  if (Boost_unit_test_framework_FOUND)
    message(STATUS "Boost unit_test_framework library found")
    target_link_libraries(trTest
      Boost::unit_test_framework
    )
    target_compile_definitions(trTest
      PRIVATE
      HAS_BOOST_UNIT_TEST_LIBRARY
    )
  endif()

  add_test(NAME trTest COMMAND trTest)
  add_dependencies(build_and_test trTest)

  if(DEBUG_DRT_UNDERFLOW)
    target_compile_definitions(drt
      PRIVATE
      DEBUG_DRT_UNDERFLOW=1
    )
  endif()
endif()

############################################################
# VTune ITT API
############################################################

if (VTune_FOUND)
  target_compile_definitions(drt
    PUBLIC
    HAS_VTUNE=1
  )

  target_link_libraries(drt
    PUBLIC
    VTune::VTune
  )

endif(VTune_FOUND)

if (Python3_FOUND AND BUILD_PYTHON)
  swig_lib(NAME          drt_py
           NAMESPACE     drt
           LANGUAGE      python
           I_FILE        src/TritonRoute-py.i
           SWIG_INCLUDES ${PROJECT_SOURCE_DIR}/include/drt
           SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/drt_py.py
  )

  target_include_directories(drt_py
    PUBLIC
      include
  )

  target_link_libraries(drt_py
    PUBLIC
      drt
  )

endif()

add_subdirectory(test)