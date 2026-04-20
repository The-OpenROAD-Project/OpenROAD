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

# Vendored third-party assets for the self-contained HTML report
# (issue #10201). Downloaded at configure time with a pinned SHA256 so
# the saved report renders with no CDN connection. Mirror of the
# http_file rules in //MODULE.bazel.
set(VENDOR_DIR ${CMAKE_CURRENT_BINARY_DIR}/vendor)
function(fetch_vendor_file name url sha256 out_var)
  set(dest ${VENDOR_DIR}/${name})
  if(NOT EXISTS ${dest})
    file(DOWNLOAD ${url} ${dest}
         EXPECTED_HASH SHA256=${sha256}
         SHOW_PROGRESS
         STATUS status)
    list(GET status 0 code)
    if(NOT code EQUAL 0)
      message(FATAL_ERROR "Failed to fetch ${url}: ${status}")
    endif()
  endif()
  set(${out_var} ${dest} PARENT_SCOPE)
endfunction()

fetch_vendor_file(leaflet.js
  "https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"
  "db49d009c841f5ca34a888c96511ae936fd9f5533e90d8b2c4d57596f4e5641a"
  VENDOR_LEAFLET_JS)
fetch_vendor_file(leaflet.css
  "https://unpkg.com/leaflet@1.9.4/dist/leaflet.css"
  "a7837102824184820dfa198d1ebcd109ff6d0ff9a2672a074b9a1b4d147d04c6"
  VENDOR_LEAFLET_CSS)
fetch_vendor_file(golden-layout.mjs
  "https://esm.sh/golden-layout@2.6.0/es2022/golden-layout.mjs"
  "f3ee7ee3278fef62b67fb71194a36c1720a30356a954ca3441d5cf9916468959"
  VENDOR_GOLDEN_LAYOUT_ESM)
fetch_vendor_file(goldenlayout-base.css
  "https://unpkg.com/golden-layout@2.6.0/dist/css/goldenlayout-base.css"
  "7b47240817e5a2cd1e24fd1f47e0933dee723b8bd99d220f27a352af2f92477b"
  VENDOR_GOLDEN_LAYOUT_BASE_CSS)
fetch_vendor_file(goldenlayout-dark-theme.css
  "https://unpkg.com/golden-layout@2.6.0/dist/css/themes/goldenlayout-dark-theme.css"
  "818c38af69eec4ed9036c11f93cde774908892734a355dd867666aa1a900523e"
  VENDOR_GOLDEN_LAYOUT_DARK_CSS)

set(REPORT_ASSETS_CPP ${CMAKE_CURRENT_BINARY_DIR}/report_assets.cpp)
add_custom_command(
  OUTPUT ${REPORT_ASSETS_CPP}
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/src/embed_report_assets.py
    --output ${REPORT_ASSETS_CPP}
    --css ${CMAKE_CURRENT_SOURCE_DIR}/src/style.css
    --leaflet-js ${VENDOR_LEAFLET_JS}
    --leaflet-css ${VENDOR_LEAFLET_CSS}
    --golden-layout-esm ${VENDOR_GOLDEN_LAYOUT_ESM}
    --golden-layout-base-css ${VENDOR_GOLDEN_LAYOUT_BASE_CSS}
    --golden-layout-dark-css ${VENDOR_GOLDEN_LAYOUT_DARK_CSS}
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
        ${CMAKE_CURRENT_SOURCE_DIR}/src/drc-widget.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main.js
  DEPENDS
    src/embed_report_assets.py
    src/style.css
    ${VENDOR_LEAFLET_JS}
    ${VENDOR_LEAFLET_CSS}
    ${VENDOR_GOLDEN_LAYOUT_ESM}
    ${VENDOR_GOLDEN_LAYOUT_BASE_CSS}
    ${VENDOR_GOLDEN_LAYOUT_DARK_CSS}
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
    src/drc-widget.js
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
