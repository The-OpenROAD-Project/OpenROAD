cmake_minimum_required(VERSION 3.9)

project(TritonRoute
  LANGUAGES CXX
)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT error)

if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7)
  message(FATAL_ERROR "TritonRoute requires GCC 7 and above! \nuse -DCMAKE_CXX_COMPILER to assign your compiler path")
endif()

# Default to bulding optimized/release executable.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RELEASE)
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)

set(FLEXROUTE_HOME ${PROJECT_SOURCE_DIR} )

## configure settings
find_package(Boost 1.68.0 COMPONENTS unit_test_framework)
find_package(OpenMP REQUIRED)
find_package(Threads REQUIRED)
find_package(VTune)

set (FLEXROUTE_SRC
  ${FLEXROUTE_HOME}/src/gr/FlexGRCMap.cpp
  ${FLEXROUTE_HOME}/src/gr/FlexGR.cpp
  ${FLEXROUTE_HOME}/src/gr/FlexGR_end.cpp
  ${FLEXROUTE_HOME}/src/gr/FlexGRGridGraph.cpp
  ${FLEXROUTE_HOME}/src/gr/FlexGRGridGraph_maze.cpp
  ${FLEXROUTE_HOME}/src/gr/FlexGR_init.cpp
  ${FLEXROUTE_HOME}/src/gr/FlexGR_maze.cpp
  ${FLEXROUTE_HOME}/src/gr/FlexGR_rq.cpp
  ${FLEXROUTE_HOME}/src/gr/FlexGR_topo.cpp
  ${FLEXROUTE_HOME}/src/dr/FlexDR_conn.cpp
  ${FLEXROUTE_HOME}/src/dr/FlexDR_init.cpp
  ${FLEXROUTE_HOME}/src/dr/FlexDR.cpp
  ${FLEXROUTE_HOME}/src/dr/FlexDR_maze.cpp
  ${FLEXROUTE_HOME}/src/dr/FlexGridGraph_maze.cpp
  ${FLEXROUTE_HOME}/src/dr/FlexGridGraph.cpp
  ${FLEXROUTE_HOME}/src/dr/FlexDR_rq.cpp
  ${FLEXROUTE_HOME}/src/dr/FlexDR_end.cpp
  ${FLEXROUTE_HOME}/src/dr/FlexDR_graphics.cpp
  ${FLEXROUTE_HOME}/src/ta/FlexTA_end.cpp
  ${FLEXROUTE_HOME}/src/ta/FlexTA_init.cpp
  ${FLEXROUTE_HOME}/src/ta/FlexTA_rq.cpp
  ${FLEXROUTE_HOME}/src/ta/FlexTA_assign.cpp
  ${FLEXROUTE_HOME}/src/ta/FlexTA.cpp
  ${FLEXROUTE_HOME}/src/global.cpp
  ${FLEXROUTE_HOME}/src/gc/FlexGC_end.cpp
  ${FLEXROUTE_HOME}/src/gc/FlexGC_rq.cpp
  ${FLEXROUTE_HOME}/src/gc/FlexGC.cpp
  ${FLEXROUTE_HOME}/src/gc/FlexGC_init.cpp
  ${FLEXROUTE_HOME}/src/gc/FlexGC_main.cpp
  ${FLEXROUTE_HOME}/src/db/drObj/drShape.cpp
  ${FLEXROUTE_HOME}/src/db/drObj/drVia.cpp
  ${FLEXROUTE_HOME}/src/db/infra/frBox.cpp
  ${FLEXROUTE_HOME}/src/db/infra/frTime_helper.cpp
  ${FLEXROUTE_HOME}/src/db/infra/frTime.cpp
  ${FLEXROUTE_HOME}/src/db/infra/frPoint.cpp
  ${FLEXROUTE_HOME}/src/db/taObj/taShape.cpp
  ${FLEXROUTE_HOME}/src/db/taObj/taTrack.cpp
  ${FLEXROUTE_HOME}/src/db/obj/frShape.cpp
  ${FLEXROUTE_HOME}/src/db/obj/frInst.cpp
  ${FLEXROUTE_HOME}/src/db/obj/frVia.cpp
  ${FLEXROUTE_HOME}/src/db/obj/frAccess.cpp
  ${FLEXROUTE_HOME}/src/db/obj/frRPin.cpp
  ${FLEXROUTE_HOME}/src/db/obj/frNode.cpp
  ${FLEXROUTE_HOME}/src/db/obj/frInstTerm.cpp
  ${FLEXROUTE_HOME}/src/frRegionQuery.cpp
  ${FLEXROUTE_HOME}/src/io/io_pin.cpp
  ${FLEXROUTE_HOME}/src/io/io.cpp
  ${FLEXROUTE_HOME}/src/io/io_guide.cpp
  ${FLEXROUTE_HOME}/src/io/io_parser_helper.cpp
  ${FLEXROUTE_HOME}/src/io/defw.cpp
  ${FLEXROUTE_HOME}/src/pa/FlexPA_init.cpp
  ${FLEXROUTE_HOME}/src/pa/FlexPA.cpp
  ${FLEXROUTE_HOME}/src/pa/FlexPA_prep.cpp
  ${FLEXROUTE_HOME}/src/pa/FlexPA_graphics.cpp
  ${FLEXROUTE_HOME}/src/rp/FlexRP_init.cpp
  ${FLEXROUTE_HOME}/src/rp/FlexRP.cpp
  ${FLEXROUTE_HOME}/src/rp/FlexRP_prep.cpp
  ${FLEXROUTE_HOME}/src/TritonRoute.cpp
  ${FLEXROUTE_HOME}/src/MakeTritonRoute.cpp
  )

############################################################
# Create TCL Interface
############################################################

set(TRITONROUTE_SWIG_FILES
    ${FLEXROUTE_HOME}/src/TritonRoute.i
)

set_source_files_properties(${TRITONROUTE_SWIG_FILES}
    PROPERTIES CPLUSPLUS ON
)

swig_add_library(tritonroute-swig
    LANGUAGE tcl
    TYPE STATIC
    SOURCES ${TRITONROUTE_SWIG_FILES}
)

set_property(TARGET tritonroute-swig PROPERTY SWIG_COMPILE_OPTIONS -namespace -prefix tr)

target_include_directories(tritonroute-swig
  PRIVATE
  ${FLEXROUTE_HOME}/include
  ${OPENROAD_HOME}/include
  ${FLEXROUTE_HOME}/src
  ${TCL_INCLUDE_PATH}
)

set(TRITONROUTE_TCL_INIT ${CMAKE_CURRENT_BINARY_DIR}/TritonRouteTclInitVar.cc)

add_custom_command(OUTPUT ${TRITONROUTE_TCL_INIT}
  COMMAND ${OPENSTA_HOME}/etc/TclEncode.tcl ${TRITONROUTE_TCL_INIT} triton_route_tcl_inits TritonRoute.tcl
  WORKING_DIRECTORY ${TRITONROUTE_HOME}/src
  DEPENDS ${TRITONROUTE_HOME}/src/TritonRoute.tcl
          ${OPENSTA_HOME}/etc/TclEncode.tcl
  )

############################################################
# Create a library
############################################################
add_library( TritonRouteLib
  ${FLEXROUTE_SRC}
  ${TRITONROUTE_TCL_INIT}
)

set_target_properties( TritonRouteLib
  PROPERTIES OUTPUT_NAME "flexroute"
)

target_include_directories( TritonRouteLib
  PUBLIC
  ${FLEXROUTE_HOME}/include
  ${TCL_INCLUDE_PATH}

  PRIVATE
  ${FLEXROUTE_HOME}/src
  ${OPENROAD_HOME}/include
  ${OPENSTA_HOME}/include
  ${OPENDB_HOME}/include
)

target_link_libraries( TritonRouteLib
  PUBLIC
  tritonroute-swig
  def
  lef
  gui
  opendb
  flute
  Threads::Threads
  OpenMP::OpenMP_CXX
  Boost::boost
  ZLIB::ZLIB
  utility
)

############################################################
# Compiler flags
############################################################
# Todo: add -Wextra and cleanup warnings
target_compile_options( TritonRouteLib
  PRIVATE
  $<$<CXX_COMPILER_ID:GNU>:-Wall -pedantic -Wcast-qual -Wredundant-decls -Wformat-security>
  $<$<CXX_COMPILER_ID:AppleClang>:-Wall -pedantic -Wcast-qual -Wredundant-decls -Wformat-security>
  $<$<CXX_COMPILER_ID:Clang>:-Wall -pedantic -Wcast-qual -Wredundant-decls -Wformat-security>
  )

############################################################
# Unit testing
############################################################
enable_testing()

add_executable(trTest
  ${FLEXROUTE_HOME}/test/gcTest.cpp
  ${FLEXROUTE_HOME}/test/fixture.cpp
)

target_include_directories( trTest
  PRIVATE
  ${FLEXROUTE_HOME}/src
  ${OPENROAD_HOME}/include
)

target_link_libraries(trTest
  TritonRouteLib
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

############################################################
# VTune ITT API
############################################################

if (VTune_FOUND)
  target_compile_definitions( TritonRouteLib
    PUBLIC
    HAS_VTUNE=1
  )

  target_link_libraries( TritonRouteLib
    PUBLIC
    VTune::VTune
  )

endif(VTune_FOUND)
