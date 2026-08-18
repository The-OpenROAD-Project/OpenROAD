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

set(WEB_ASSET_FILES
    src/index.html
    src/style.css
    src/theme.js
    src/tile-merge.js
    src/tile-request.js
    src/device-pixels.js
    src/merged-tile-layer.js
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
    src/title.js
    src/clock-tree-widget.js
    src/schematic-widget.js
    src/openroad_skin.svg
    src/charts-widget.js
    src/timing-widget.js
    src/drc-widget.js
    src/3d-viewer-widget.js
    src/main.js
)

# The browser libraries are not checked in: the build fetches them from the npm
# registry, verified against the digests in third-party/packages.json, and
# bundles golden-layout, which publishes no browser build (issue #11065).  Bazel
# does the same through //bazel:web_third_party.bzl.  This runs at configure
# time and only does work when the manifest changed.
#
# Point WEB_THIRD_PARTY_TARBALL_DIR at a directory primed with
#   third-party/fetch_packages.py --download-only --tarball-dir <dir>
# to configure with no network.
set(WEB_THIRD_PARTY_TARBALL_DIR "" CACHE PATH
    "Pre-downloaded npm tarballs for the web viewer's browser libraries")
set(WEB_THIRD_PARTY_DIR ${CMAKE_CURRENT_BINARY_DIR}/third-party)
set(WEB_THIRD_PARTY_CMAKE ${CMAKE_CURRENT_BINARY_DIR}/web_third_party.cmake)

set(WEB_FETCH_ARGS --dest ${WEB_THIRD_PARTY_DIR} --emit-cmake ${WEB_THIRD_PARTY_CMAKE})
if(WEB_THIRD_PARTY_TARBALL_DIR)
  list(APPEND WEB_FETCH_ARGS --tarball-dir ${WEB_THIRD_PARTY_TARBALL_DIR})
endif()

execute_process(
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/third-party/fetch_packages.py ${WEB_FETCH_ARGS}
  RESULT_VARIABLE WEB_FETCH_RESULT
)
if(NOT WEB_FETCH_RESULT EQUAL 0)
  message(FATAL_ERROR
    "failed to fetch the web viewer's browser libraries. Without network "
    "access, prime a directory with third-party/fetch_packages.py "
    "--download-only --tarball-dir <dir> and configure with "
    "-DWEB_THIRD_PARTY_TARBALL_DIR=<dir>.")
endif()

# Defines WEB_THIRD_PARTY_ASSET_ARGS and WEB_THIRD_PARTY_FILES.
include(${WEB_THIRD_PARTY_CMAKE})

# Each asset is passed as <served path>=<file path>.  The viewer's own assets
# are served flat ("/main.js"); the libraries keep the layout their packages
# ship ("/third-party/leaflet/images/layers.png"), because the stylesheets reach
# their icons through relative urls.
set(WEB_ASSET_ARGS "")
foreach(f ${WEB_ASSET_FILES})
  string(REGEX REPLACE "^src/" "" served ${f})
  list(APPEND WEB_ASSET_ARGS /${served}=${CMAKE_CURRENT_SOURCE_DIR}/${f})
endforeach()
list(APPEND WEB_ASSET_ARGS ${WEB_THIRD_PARTY_ASSET_ARGS})

set(REPORT_ASSETS_CPP ${CMAKE_CURRENT_BINARY_DIR}/report_assets.cpp)
add_custom_command(
  OUTPUT ${REPORT_ASSETS_CPP}
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/src/embed_report_assets.py
    --output ${REPORT_ASSETS_CPP}
    --css ${CMAKE_CURRENT_SOURCE_DIR}/src/style.css
    # Same order as the report_assets genrule in BUILD: the files are
    # concatenated into one scope, so the order is what runs.
    --js ${CMAKE_CURRENT_SOURCE_DIR}/src/theme.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/3d-viewer-widget.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/coordinates.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui-utils.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/checkbox-tree-model.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vis-tree.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/websocket-manager.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/tile-merge.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/tile-request.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/device-pixels.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/websocket-tile-layer.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/merged-tile-layer.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/display-controls.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/inspector.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ruler.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/tcl-completer.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/hierarchy-browser.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/menu-bar.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/title.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/clock-tree-widget.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/schematic-widget.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/charts-widget.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/timing-widget.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/drc-widget.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main.js
  DEPENDS
    src/embed_report_assets.py
    ${WEB_ASSET_FILES}
  COMMENT "Generating report_assets.cpp"
)

set(WEB_ASSETS_CPP ${CMAKE_CURRENT_BINARY_DIR}/web_assets.cpp)
add_custom_command(
  OUTPUT ${WEB_ASSETS_CPP}
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/src/embed_web_assets.py
    --output ${WEB_ASSETS_CPP}
    ${WEB_ASSET_ARGS}
  DEPENDS
    src/embed_web_assets.py
    ${WEB_ASSET_FILES}
    ${WEB_THIRD_PARTY_FILES}
  COMMENT "Generating web_assets.cpp"
)

target_sources(web
  PRIVATE
    src/clock_tree_report.cpp
    src/color.cpp
    src/hierarchy_report.cpp
    src/request_handler.cpp
    src/search.cpp
    src/font_data.cpp
    src/glyph_cache.cpp
    src/tile_generator.cpp
    src/timing_report.cpp
    src/web.cpp
    src/web_serve.cpp
    src/web_chart.cpp
    src/web_painter.cpp
    src/web_viewer_hook.cpp
    src/MakeWeb.cpp
    ${REPORT_ASSETS_CPP}
    ${WEB_ASSETS_CPP}
)

target_link_libraries(web
  PUBLIC
    odb
  PRIVATE
    cli_completer
    gui_descriptors
    gui_heatmap_core
    dbSta_lib
    lodepng
    stb_truetype
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

target_compile_definitions(web
  PRIVATE
    BOOST_ASIO_NO_DEPRECATED
    BOOST_JSON_HEADER_ONLY
)

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
