# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

include("openroad")

swig_lib(NAME         web
         NAMESPACE    web
         I_FILE       src/web.i
         SCRIPTS      ${CMAKE_CURRENT_SOURCE_DIR}/src/web.tcl
         SWIG_INCLUDES ${ODB_HOME}/src/swig/common
                       ${ODB_HOME}/src/swig/tcl
                       ${ODB_HOME}/include
)

target_sources(web
  PRIVATE
    src/clock_tree_report.cpp
    src/color.cpp
    src/hierarchy_report.cpp
    src/request_handler.cpp
    src/search.cpp
    src/tile_generator.cpp
    src/timing_report.cpp
    src/web.cpp
    src/MakeWeb.cpp
)

target_link_libraries(web
  PUBLIC
    odb
  PRIVATE
    gui_descriptors
    gui_heatmap_core
    dbSta_lib
    lodepng
    utl_lib
)

target_include_directories(web
  PUBLIC
    include
  PRIVATE
    src
    ${TCL_INCLUDE_PATH}
    ${OPENSTA_HOME}/include
    ${DBSTA_HOME}/include
    ${OPENROAD_HOME}/third-party
)

target_compile_definitions(web PRIVATE BOOST_ASIO_NO_DEPRECATED)

messages(
  TARGET web
)

################################################################

if (Python3_FOUND AND BUILD_PYTHON)
  swig_lib(NAME          web_py
           NAMESPACE     web
           LANGUAGE      python
           I_FILE        src/web-py.i
           SWIG_INCLUDES ${PROJECT_SOURCE_DIR}/include/dpl
                         ${ODB_HOME}/src/swig/common
                         ${ODB_HOME}/src/swig/python
           SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/web_py.py
  )

  target_link_libraries(web_py
    PUBLIC
      web
  )

endif()

if(ENABLE_TESTS)
  add_subdirectory(test)
endif()
