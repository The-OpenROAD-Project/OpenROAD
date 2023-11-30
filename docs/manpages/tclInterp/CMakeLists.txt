cmake_minimum_required(VERSION 3.16)
project(AAAA)

################################################################

# OSX tcl is deprecated and prints a warning, so look for a user installed
# version before using the system version.
# I tried to override the library search order instead but failed.
# CMAKE_FIND_FRAMEWORK LAST bypasses the version in the framework directory
# but not the one in /usr/lib.
# This calls cmake/FindTCL.cmake
# Do not use REQUIRED because it also requires TK.
find_package(TCL)

add_executable(main.o main.cpp)

# check for tclReadline
set(TCL_READLINE_POSSIBLE_NAMES tclreadline-2.1.0
    tclreadline-2.3.2 tclreadline-2.3.6 tclreadline-2.3.7 tclreadline-2.3.8
    )
find_library(TCL_READLINE_LIBRARY
  NAMES tclreadline ${TCL_READLINE_POSSIBLE_NAMES}
  PATHS ${TCL_LIB_PATHS}
  )
if (TCL_READLINE_LIBRARY)
  message(STATUS "TCL readline library: ${TCL_READLINE_LIBRARY}")
endif()

find_path(TCL_READLINE_H tclreadline.h)
if (TCL_READLINE_H)
  message(STATUS "TCL readline header: ${TCL_READLINE_H}")
endif()

option(BUILD_TCLX "Build with tclX included" ON)
set(TCLX_LIBRARY /usr/lib/libtclx8.4.so.0)
set(TCLX_H /usr/include/tclx8.4/tclExtend.h)
if (BUILD_TCLX AND TCLX_LIBRARY)
  message(STATUS "TclX library: ${TCLX_LIBRARY}")
endif()
if (BUILD_TCLX AND TCLX_H)
  message(STATUS "TclX header: ${TCLX_H}")
endif()

target_link_libraries(main.o  
                 ${TCL_LIBRARY}
                 ${TCL_READLINE_LIBRARY})

include_directories(/usr/include/tclx8.4)

if (BUILD_TCLX AND TCLX_LIBRARY AND TCLX_H)
  target_link_libraries(main.o ${TCLX_LIBRARY})
  #target_include_directories(main.o ${TCLX_H})
endif()