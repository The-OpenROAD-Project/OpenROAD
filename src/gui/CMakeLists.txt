find_package(Qt5 COMPONENTS Core Widgets)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

if (Qt5_FOUND)
  message(STATUS "GUI is enabled")
  set(CMAKE_AUTOMOC ON)

  add_library(gui
    src/layoutViewer.cpp
    src/mainWindow.cpp
    src/scriptWidget.cpp
    src/displayControls.cpp
    src/gui.cpp
    src/search.cpp 
  )

  target_link_libraries(gui
    opendb
    Qt5::Core
    Qt5::Widgets
  )
else()
  message(STATUS "GUI is not enabled")
  add_library(gui src/stub.cpp)
endif()

target_include_directories(gui
  PUBLIC
    ${GUI_HOME}/include
  PRIVATE
    ${OPENROAD_HOME}/include
    ${Boost_INCLUDE_DIRS}
)

