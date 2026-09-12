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
    src/display-state.js
    src/coordinates.js
    src/ui-utils.js
    src/checkbox-tree-model.js
    src/vis-tree.js
    src/websocket-manager.js
    src/websocket-tile-layer.js
    src/display-controls.js
    src/inspector.js
    src/ruler.js
    src/label-manager.js
    src/tcl-completer.js
    src/hierarchy-browser.js
    src/menu-bar.js
    src/context-menu.js
    src/search-nav.js
    src/capture.js
    src/toolbar.js
    src/image-export.js
    src/edit-dialogs.js
    src/title.js
    src/clock-tree-widget.js
    src/schematic-widget.js
    src/openroad_skin.svg
    src/charts-widget.js
    src/timing-widget.js
    src/drc-widget.js
    src/3d-viewer-widget.js
    src/selection-browser.js
    src/main.js
    # The vendored browser libraries.  They are served from the binary like any
    # other asset so the viewer never loads code from a CDN (issue #11065); see
    # third-party/README.md.  Their directory layout is part of the contract:
    # the stylesheets reach their icons through relative urls.
    third-party/leaflet/leaflet.js
    third-party/leaflet/leaflet.css
    third-party/leaflet/images/layers.png
    third-party/leaflet/images/layers-2x.png
    third-party/leaflet/images/marker-icon.png
    third-party/leaflet/images/marker-icon-2x.png
    third-party/leaflet/images/marker-shadow.png
    third-party/golden-layout/golden-layout.esm.js
    third-party/golden-layout/css/goldenlayout-base.css
    third-party/golden-layout/css/themes/goldenlayout-dark-theme.css
    third-party/golden-layout/css/themes/goldenlayout-light-theme.css
    third-party/golden-layout/img/lm_close_black.png
    third-party/golden-layout/img/lm_close_tab_white.png
    third-party/golden-layout/img/lm_close_white.png
    third-party/golden-layout/img/lm_maximise_black.png
    third-party/golden-layout/img/lm_maximise_white.png
    third-party/golden-layout/img/lm_minimize_black.png
    third-party/golden-layout/img/lm_minimize_white.png
    third-party/golden-layout/img/lm_popin_black.png
    third-party/golden-layout/img/lm_popin_white.png
    third-party/golden-layout/img/lm_popout_black.png
    third-party/golden-layout/img/lm_popout_white.png
    third-party/three/three.module.min.js
    third-party/elkjs/elk.bundled.js
    third-party/netlistsvg/netlistsvg.bundle.js
)

# Each asset is passed as <served path>=<file path>.  Assets under src/ are
# served flat ("/main.js"); everything else keeps its path
# ("/third-party/leaflet/images/layers.png").
set(WEB_ASSET_ARGS "")
foreach(f ${WEB_ASSET_FILES})
  string(REGEX REPLACE "^src/" "" served ${f})
  list(APPEND WEB_ASSET_ARGS /${served}=${CMAKE_CURRENT_SOURCE_DIR}/${f})
endforeach()

set(REPORT_ASSETS_CPP ${CMAKE_CURRENT_BINARY_DIR}/report_assets.cpp)
add_custom_command(
  OUTPUT ${REPORT_ASSETS_CPP}
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/src/embed_report_assets.py
    --output ${REPORT_ASSETS_CPP}
    --css ${CMAKE_CURRENT_SOURCE_DIR}/src/style.css
    # Same order as the report_assets genrule in BUILD: the files are
    # concatenated into one scope, so the order is what runs.
    --js ${CMAKE_CURRENT_SOURCE_DIR}/src/theme.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/display-state.js
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
        ${CMAKE_CURRENT_SOURCE_DIR}/src/label-manager.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/tcl-completer.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/hierarchy-browser.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/menu-bar.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/context-menu.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/search-nav.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/capture.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/toolbar.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/image-export.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/edit-dialogs.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/title.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/clock-tree-widget.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/schematic-widget.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/charts-widget.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/timing-widget.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/drc-widget.js
        ${CMAKE_CURRENT_SOURCE_DIR}/src/selection-browser.js
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
    src/web_gif.cpp
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
