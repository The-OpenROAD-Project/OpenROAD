# If Qt is not installed there will not be cmake
# support for the package so this needs to be "quiet".
find_package(Qt5 QUIET COMPONENTS Core Widgets)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

if (Qt5_FOUND)
  message(STATUS "GUI is enabled")
  set(CMAKE_AUTOMOC ON)
  set(CMAKE_AUTOUIC ON)
  set(CMAKE_AUTOUIC_SEARCH_PATHS ui)

  set(GUI_WRAP ${CMAKE_CURRENT_BINARY_DIR}/gui_wrap.cc)

  add_custom_command(OUTPUT ${GUI_WRAP}
    COMMAND ${SWIG_EXECUTABLE} -tcl8 -c++ -namespace -prefix gui
            -I${OPENROAD_HOME}/include -o ${GUI_WRAP} ${GUI_HOME}/src/gui.i
    COMMAND ${OPENSTA_HOME}/etc/SwigCleanup.tcl ${GUI_WRAP}
    WORKING_DIRECTORY ${GUI_HOME}/src
    DEPENDS
      ${GUI_HOME}/src/gui.i
      ${GUI_HOME}/include/gui/gui.h
  )

  set_property(SOURCE ${GUI_WRAP} PROPERTY SKIP_AUTOMOC ON)

  add_library(gui
    src/layoutViewer.cpp
    src/mainWindow.cpp
    src/scriptWidget.cpp
    src/displayControls.cpp
    src/gui.cpp
    src/search.cpp 
    src/findDialog.cpp
    src/congestionSetupDialog.cpp
    src/highlightGroupDialog.cpp
    src/selectHighlightWindow.cpp
    ${GUI_WRAP}
  )

#qt5_wrap_ui(findDlg.ui selHltWidget.ui selectedWidget.ui congestionSetup.ui)

  target_link_libraries(gui
    opendb
    Qt5::Core
    Qt5::Widgets
    utility
  )
else()
  message(STATUS "GUI is not enabled")
  add_library(gui src/stub.cpp)
endif()

target_include_directories(gui
  PUBLIC
    ${GUI_HOME}/include
    ${OPENDB_HOME}/include
  PRIVATE
    ${OPENROAD_HOME}/include
    ${OPENSTA_HOME}/include
    ${FASTROUTE_HOME}/include
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}
    ${AUTOGEN_BUILD_DIR}/include
)
