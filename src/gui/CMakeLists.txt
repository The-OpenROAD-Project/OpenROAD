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

# Lightweight library containing only DescriptorRegistry, Property::toString,
# and Selected methods.  Has no Qt, no dbDescriptors, no ord dependency.
# Used by the web module so its test doesn't pull in the full GUI.
add_library(gui_descriptors
  src/descriptor_registry.cpp
)
target_link_libraries(gui_descriptors
  PUBLIC
    odb
  PRIVATE
    utl_lib
)
target_include_directories(gui_descriptors
  PUBLIC
    include
  PRIVATE
    ${OPENSTA_HOME}/include
    ${DBSTA_HOME}/include
)

add_library(gui_heatmap_core
  src/heatMapCore.cpp
  src/heatMapPinDensity.cpp
  src/heatMapPlacementDensity.cpp
)
target_link_libraries(gui_heatmap_core
  PUBLIC
    odb
  PRIVATE
    absl::synchronization
    dbSta_lib
    utl_lib
    Boost::boost
    OpenSTA
)
target_include_directories(gui_heatmap_core
  PUBLIC
    include
  PRIVATE
    src
    ${OPENSTA_HOME}/include
    ${DBSTA_HOME}/include
)

if (Qt5_FOUND AND BUILD_GUI)
  message(STATUS "GUI is enabled")
  set(CMAKE_AUTOMOC ON)
  set(CMAKE_AUTORCC ON)
  set(CMAKE_AUTOUIC ON)
  set(CMAKE_AUTOUIC_SEARCH_PATHS qt/ui)

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
    qt/src/layoutViewer.cpp
    qt/src/layoutTabs.cpp
    qt/src/renderThread.cpp
    qt/src/painter.cpp
    qt/src/mainWindow.cpp
    qt/src/scriptWidget.cpp
    qt/src/cmdInputWidget.cpp
    qt/src/tclCmdInputWidget.cpp
    qt/src/tclCmdHighlighter.cpp
    qt/src/displayControls.cpp
    qt/src/gui.cpp
    qt/src/search.cpp
    qt/src/findDialog.cpp
    qt/src/gotoDialog.cpp
    qt/src/inspector.cpp
    src/init_descriptors.cpp
    src/dbDescriptors.cpp
    src/staDescriptors.cpp
    qt/src/highlightGroupDialog.cpp
    qt/src/selectHighlightWindow.cpp
    qt/src/staGui.cpp
    qt/src/staGuiInterface.cpp
    qt/src/timingWidget.cpp
    qt/src/drcWidget.cpp
    qt/src/helpWidget.cpp
    src/ruler.cpp
    qt/src/label.cpp
    qt/src/heatMap.cpp
    qt/src/heatMapSetup.cpp
    qt/src/browserWidget.cpp
    qt/src/globalConnectDialog.cpp
    qt/src/insertBufferDialog.cpp
    qt/src/clockWidget.cpp
    qt/src/gui_utils.cpp
    qt/src/colorGenerator.cpp
    qt/src/bufferTreeDescriptor.cpp
    qt/src/chartsWidget.cpp
    qt/src/chiplet3DWidget.cpp
    qt/src/GUIProgress.cpp
    qt/src/dropdownCheckboxes.cpp
    qt/resources/resource.qrc
  )

  target_compile_definitions(gui PRIVATE ENABLE_QT)

  target_link_libraries(gui
    PUBLIC
      odb
      PRIVATE
      absl::synchronization
      Qt5::Core
      Qt5::Widgets
      Qt5::Charts
      OpenGL::GL
      gui_descriptors
      gui_heatmap_core
      dbSta
      utl_lib
      Boost::boost
      OpenSTA
  )

  target_include_directories(gui
    PRIVATE
      src
      qt/src
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
    src/init_descriptors.cpp
    src/dbDescriptors.cpp
    src/staDescriptors.cpp
  )
  target_link_libraries(gui
    PUBLIC
      odb
    PRIVATE
      gui_descriptors
      gui_heatmap_core
      dbSta
      utl_lib
      OpenSTA
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

  install(FILES qt/resources/icon.png DESTINATION ${OPENROAD_SHARE_GUI_DIR})
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/resources/openroad.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications/)
endif()
