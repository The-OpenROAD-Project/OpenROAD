# If Qt is not installed there will not be cmake
# support for the package so this needs to be "quiet".
find_package(Qt5 QUIET COMPONENTS Core Widgets)

include("openroad")
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if (Qt5_FOUND)
  message(STATUS "GUI is enabled")
  set(CMAKE_AUTOMOC ON)
  set(CMAKE_AUTOUIC ON)
  set(CMAKE_AUTOUIC_SEARCH_PATHS ui)

  set(GUI_WRAP ${CMAKE_CURRENT_BINARY_DIR}/gui_wrap.cc)

  tcl_lib(NAME      gui
          NAMESPACE gui
          I_FILE    src/gui.i
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

  target_link_libraries(gui
    PUBLIC
      opendb
    PRIVATE
      Qt5::Core
      Qt5::Widgets
      utility
      Boost::boost
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
