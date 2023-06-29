# Allow users to not build without the gui even if Qt is found
option(BUILD_GUI "Build the GUI" ON)

# If Qt is not installed there will not be cmake
# support for the package so this needs to be "quiet".
find_package(Qt5 QUIET COMPONENTS Core Widgets OPTIONAL_COMPONENTS Charts)

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

  if (TARGET Qt5::Charts)
    message(STATUS "Charts widget is enabled")

    target_compile_definitions(gui PRIVATE ENABLE_CHARTS)

    list(APPEND CHARTS_SRC
      "src/chartsWidget.cpp"
    )

    list(APPEND CHARTS_LIB
      "Qt5::Charts"
    )
  else()
    message(STATUS "Charts widget is not enabled")
  endif()

  set_property(SOURCE ${GUI_WRAP} PROPERTY SKIP_AUTOMOC ON)

  target_sources(gui
   PRIVATE
    src/layoutViewer.cpp
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
    src/highlightGroupDialog.cpp
    src/selectHighlightWindow.cpp
    src/staGui.cpp
    src/staGuiInterface.cpp
    src/timingWidget.cpp
    src/drcWidget.cpp
    src/ruler.cpp
    src/heatMap.cpp
    src/heatMapSetup.cpp
    src/heatMapPlacementDensity.cpp
    src/browserWidget.cpp
    src/globalConnectDialog.cpp
    src/clockWidget.cpp
    src/gui_utils.cpp
    src/colorGenerator.cpp
    src/bufferTreeDescriptor.cpp
    ${CHARTS_SRC}
    resources/resource.qrc
  )

  target_link_libraries(gui
    PUBLIC
      odb
    PRIVATE
      Qt5::Core
      Qt5::Widgets
      ${CHARTS_LIB}
      utl
      Boost::boost
  )

messages(
  TARGET gui
)

else()
  message(STATUS "GUI is not enabled")
  add_library(gui 
    src/stub.cpp 
    src/stub_heatMap.cpp
  )
  target_link_libraries(gui
    PUBLIC
      odb
  )
endif()

target_include_directories(gui
  PUBLIC
    include
  PRIVATE
    ${OPENSTA_HOME}/include
    ${DBSTA_HOME}/include
)
