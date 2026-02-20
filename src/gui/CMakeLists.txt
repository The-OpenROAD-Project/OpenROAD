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
    src/dbDescriptors.cpp
    src/staDescriptors.cpp
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
    src/heatMapPinDensity.cpp
    src/heatMapPlacementDensity.cpp
    src/browserWidget.cpp
    src/globalConnectDialog.cpp
    src/insertBufferDialog.cpp
    src/clockWidget.cpp
    src/gui_utils.cpp
    src/colorGenerator.cpp
    src/bufferTreeDescriptor.cpp
    src/chartsWidget.cpp
    src/chiplet3DWidget.cpp
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
  else()
    message(STATUS "GUI is not enabled")
  endif()
  add_library(gui
    src/stub.cpp
    src/stub_heatMap.cpp
  )
  target_link_libraries(gui
    PUBLIC
      odb
  )
  target_include_directories(gui
    PRIVATE
      ${OPENROAD_HOME}/third-party/gif-h
  )
endif()

target_include_directories(gui
  PUBLIC
    include
  PRIVATE
    ${OPENSTA_HOME}/include
    ${DBSTA_HOME}/include
    ${OPENROAD_HOME}/third-party/gif-h
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
