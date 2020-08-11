find_package(Qt5 COMPONENTS Core Widgets REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(gui
  src/layoutViewer.cpp
  src/mainWindow.cpp
  src/scriptWidget.cpp
  src/displayControls.cpp
  src/gui.cpp
  src/search.cpp 
)

target_include_directories(gui
  PUBLIC
    ${GUI_HOME}/include
  PRIVATE
    ${OPENROAD_HOME}/include
    ${Boost_INCLUDE_DIRS}
)

target_link_libraries(gui
  opendb
  Qt5::Core
  Qt5::Widgets
)

