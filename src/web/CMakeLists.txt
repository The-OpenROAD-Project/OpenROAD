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

set(REPORT_ASSETS_CPP ${CMAKE_CURRENT_BINARY_DIR}/report_assets.cpp)
add_custom_command(
  OUTPUT ${REPORT_ASSETS_CPP}
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/src/embed_report_assets.py
    --output ${REPORT_ASSETS_CPP}
    --css ${CMAKE_CURRENT_SOURCE_DIR}/src/style.css
    --js ${CMAKE_CURRENT_SOURCE_DIR}/src/theme.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/coordinates.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui-utils.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/checkbox-tree-model.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vis-tree.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/websocket-manager.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/websocket-tile-layer.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/display-controls.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/inspector.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ruler.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/tcl-completer.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/hierarchy-browser.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/menu-bar.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/clock-tree-widget.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/schematic-widget.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/charts-widget.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/timing-widget.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main.js
  DEPENDS
    src/embed_report_assets.py
    src/style.css
    src/theme.js
    src/coordinates.js
    src/ui-utils.js
    src/checkbox-tree-model.js
    src/vis-tree.js
    src/websocket-manager.js
    src/websocket-tile-layer.js
    src/display-controls.js
    src/inspector.js
    src/ruler.js
    src/tcl-completer.js
    src/hierarchy-browser.js
    src/menu-bar.js
    src/clock-tree-widget.js
    src/schematic-widget.js
    src/charts-widget.js
    src/timing-widget.js
    src/main.js
  COMMENT "Generating report_assets.cpp"
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
    ${REPORT_ASSETS_CPP}
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
