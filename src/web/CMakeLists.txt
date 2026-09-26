# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

include("openroad")

find_package(ZLIB REQUIRED)

# The Qt-free half of what used to be the gui: the data model every other
# module draws through -- Painter, Renderer, Descriptor/Selected,
# HeatMapDataSource -- published through gui/core.h.  It lives here because
# the web viewer is its main consumer and nothing in it needs Qt; the Qt gui
# links it too, so a binary carries one copy either way.  Mirrors
# //src/web:core in the Bazel build; keep the two source lists in step.
#
# Not complete on its own.  It carries Gui's dispatch spine and everything a
# module outside gui calls, but the Qt gui's own Tcl command surface stays in
# gui.cpp, and the startGui/initGui entry points are implemented once per
# build -- gui.cpp for a Qt one, makeGui.cpp for a CLI one -- so a binary has
# to link one of those too.
#
# This target does not take the gui target's LTO opt-out.  That one is there
# for a crash in Qt plugin loading, and nothing here is Qt: no
# QApplication, no static QPA plugin, nothing that takes part in loading one.
# descriptor_registry.cpp and the heatmap sources have also been built with
# LTO all along, through the gui_descriptors and gui_heatmap_core targets this
# replaces, and they are on the web tile renderer's hot path.
add_library(gui_core
  src/bufferTreeDescriptor.cpp
  src/colorGenerator.cpp
  src/dbDescriptors.cpp
  src/descriptor_registry.cpp
  src/heatMapCore.cpp
  src/heatMapPinDensity.cpp
  src/heatMapPlacementDensity.cpp
  src/gifWriter.cpp
  src/guiDispatch.cpp
  src/init_descriptors.cpp
  src/renderer.cpp
  src/staDescriptors.cpp
)
target_link_libraries(gui_core
  PUBLIC
    odb
  PRIVATE
    absl::synchronization
    dbSta_lib
    utl_lib
    Boost::boost
    OpenSTA
)
target_include_directories(gui_core
  PUBLIC
    include
    # The Qt gui's widgets include the descriptor and colour-palette headers
    # from here.  Bazel names just those four in :core's hdrs; CMake can only
    # publish a directory, so this is wider than it needs to be.
    src
  PRIVATE
    ${OPENSTA_HOME}/include
    ${DBSTA_HOME}/include
    ${OPENROAD_HOME}
)

swig_lib(NAME         web
         NAMESPACE    web
         I_FILE       src/web.i
         SCRIPTS      ${CMAKE_CURRENT_SOURCE_DIR}/src/web.tcl
         SWIG_INCLUDES ${ODB_HOME}/src/swig/common
                       ${ODB_HOME}/src/swig/tcl
                       ${ODB_HOME}/include
)

# ── The browser bundles ─────────────────────────────────────────────────────
#
# The viewer's libraries (leaflet, golden-layout, three, elkjs, netlistsvg) come
# from npm and are bundled and minified by esbuild, so the browser never fetches
# code from a CDN (issue #11065).  Only Bazel can drive that pipeline, so this
# build embeds a copy of its output checked in under dist/.  See dist/README.md
# to regenerate it.
set(WEB_BUNDLE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dist)

# Each asset is passed as <served path>=<file path>; embed_web_assets.py gzips
# them itself, so this needs no gzip on the host and both build systems embed
# identical bytes.
set(WEB_ASSET_ARGS
  /index.html=${WEB_BUNDLE_DIR}/index.min.html
  /app.min.js=${WEB_BUNDLE_DIR}/app.min.js
)

set(REPORT_ASSETS_CPP ${CMAKE_CURRENT_BINARY_DIR}/report_assets.cpp)
add_custom_command(
  OUTPUT ${REPORT_ASSETS_CPP}
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/src/embed_report_assets.py
    --output ${REPORT_ASSETS_CPP}
    --vendor-css ${WEB_BUNDLE_DIR}/vendor.min.css
    --theme-dark ${WEB_BUNDLE_DIR}/gl-dark.min.css
    --theme-light ${WEB_BUNDLE_DIR}/gl-light.min.css
    --app-css ${WEB_BUNDLE_DIR}/app.min.css
    --js ${WEB_BUNDLE_DIR}/report.min.js
  DEPENDS
    src/embed_report_assets.py
    ${WEB_BUNDLE_DIR}/vendor.min.css
    ${WEB_BUNDLE_DIR}/gl-dark.min.css
    ${WEB_BUNDLE_DIR}/gl-light.min.css
    ${WEB_BUNDLE_DIR}/app.min.css
    ${WEB_BUNDLE_DIR}/report.min.js
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
    ${WEB_BUNDLE_DIR}/index.min.html
    ${WEB_BUNDLE_DIR}/app.min.js
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
    src/asset_gzip.cpp
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
    gui_core
    dbSta_lib
    lodepng
    stb_truetype
    utl_lib
    ZLIB::ZLIB
)

# libdeflate compresses tile PNGs several times faster than lodepng's built-in
# deflate.  It is optional: without it lodepng's own compressor is used.
find_package(libdeflate CONFIG QUIET)
if (TARGET libdeflate::libdeflate_static)
  set(WEB_LIBDEFLATE libdeflate::libdeflate_static)
elseif (TARGET libdeflate::libdeflate_shared)
  set(WEB_LIBDEFLATE libdeflate::libdeflate_shared)
endif()
if (WEB_LIBDEFLATE)
  message(STATUS "web: using libdeflate for PNG compression")
  target_link_libraries(web PRIVATE ${WEB_LIBDEFLATE})
  target_compile_definitions(web PRIVATE HAVE_LIBDEFLATE)
else()
  message(STATUS "web: libdeflate not found; PNG tiles use lodepng's compressor")
endif()

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
