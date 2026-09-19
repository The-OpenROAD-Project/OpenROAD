# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2020-2025, The OpenROAD Authors

# Allow users to not build without the gui even if Qt is found
option(BUILD_GUI "Build the GUI" ON)

# If Qt is not installed there will not be cmake
# support for the package so this needs to be "quiet".
find_package(Qt5 QUIET COMPONENTS Core Widgets Charts)
find_package(OpenGL REQUIRED)

include("openroad")
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# The Qt-free half of the gui: the data model every other module draws
# through -- Painter, Renderer, Descriptor/Selected, HeatMapDataSource --
# published through gui/core.h.  Compiled once and linked by both the CLI
# and the Qt gui.  Mirrors //src/gui:core in the Bazel build; keep the two
# source lists in step.
#
# Not yet complete on its own.  It carries Gui's dispatch spine, but the Tcl
# command surface and the startGui/initGui entry points are still implemented
# twice, in gui.cpp (Qt) and stub.cpp (CLI), so a binary has to link one of
# those too.  Phase 3 is moving that surface here slice by slice.
#
# This target does not take the gui target's LTO opt-out below.  That one is
# there for a crash in Qt plugin loading, and nothing here is Qt: no
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
  PRIVATE
    src
    ${OPENSTA_HOME}/include
    ${DBSTA_HOME}/include
    ${OPENROAD_HOME}
)

if (Qt5_FOUND AND BUILD_GUI)
  message(STATUS "GUI is enabled")
  set(CMAKE_AUTOMOC ON)
  set(CMAKE_AUTORCC ON)
  set(CMAKE_AUTOUIC ON)
  set(CMAKE_AUTOUIC_SEARCH_PATHS ui)

  set(GUI_WRAP ${CMAKE_CURRENT_BINARY_DIR}/gui_wrap.cc)

  swig_lib(NAME      gui
           NAMESPACE gui
           I_FILE    src/gui.i
           SCRIPTS   src/gui.tcl
           RUNTIME_HEADER tclSwig.h
  )

  set_property(SOURCE ${GUI_WRAP} PROPERTY SKIP_AUTOMOC ON)

  target_sources(gui
   PRIVATE
    src/layoutViewer.cpp
    src/layoutTabs.cpp
    src/renderThread.cpp
    src/painter.cpp
    src/mainWindow.cpp
    src/scriptWidget.cpp
    src/cmdInputWidget.cpp
    src/tclCmdInputWidget.cpp
    src/tclCmdHighlighter.cpp
    src/displayControls.cpp
    src/gui.cpp
    src/search.cpp
    src/findDialog.cpp
    src/gotoDialog.cpp
    src/inspector.cpp
    src/highlightGroupDialog.cpp
    src/selectHighlightWindow.cpp
    src/staGui.cpp
    src/staGuiInterface.cpp
    src/timingWidget.cpp
    src/drcWidget.cpp
    src/helpWidget.cpp
    src/ruler.cpp
    src/label.cpp
    src/heatMap.cpp
    src/heatMapSetup.cpp
    src/browserWidget.cpp
    src/globalConnectDialog.cpp
    src/insertBufferDialog.cpp
    src/qtDialogs.cpp
    src/clockWidget.cpp
    src/gui_utils.cpp
    src/chartsWidget.cpp
    src/GUIProgress.cpp
    src/dropdownCheckboxes.cpp
    resources/resource.qrc
  )

  target_link_libraries(gui
    PUBLIC
      odb
      PRIVATE
      absl::synchronization
      Qt5::Core
      Qt5::Widgets
      Qt5::Charts
      OpenGL::GL
      gui_core
      dbSta
      utl_lib
      Boost::boost
      OpenSTA
  )

  # Disable LTO for gui. LTO causes a crash with Qt plugin loading
  set_target_properties(gui PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION OFF
    INTERPROCEDURAL_OPTIMIZATION_RELEASE OFF
  )
  target_compile_options(gui PRIVATE "-fno-lto")
  target_link_options(gui PRIVATE "-fno-lto")

messages(
  TARGET gui
)

else()
  if(BUILD_GUI)
    message(STATUS "QT5 not found. GUI is disabled")
    set(BUILD_GUI OFF CACHE BOOL "Build the GUI" FORCE)
  else()
    message(STATUS "GUI is not enabled")
  endif()
  add_library(gui
    src/stub.cpp
  )
  target_link_libraries(gui
    PUBLIC
      odb
    PRIVATE
      gui_core
      dbSta
      utl_lib
      OpenSTA
  )
  target_include_directories(gui
    PRIVATE
      ${OPENROAD_HOME}
  )
endif()

target_include_directories(gui
  PUBLIC
    include
  PRIVATE
    ${OPENSTA_HOME}/include
    ${DBSTA_HOME}/include
    ${OPENROAD_HOME}
)

add_subdirectory(test)

set(OPENROAD_SHARE_GUI_DIR ${OPENROAD_SHARE}/gui)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  # configure desktop file
  configure_file(
    resources/openroad.desktop.cmake
    resources/openroad.desktop
  )

  install(FILES resources/icon.png DESTINATION ${OPENROAD_SHARE_GUI_DIR})
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/resources/openroad.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications/)
endif()
