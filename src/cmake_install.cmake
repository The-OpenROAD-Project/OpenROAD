# Install script for directory: /home/stephano/OpenROAD/src

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/openroad" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/openroad")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/openroad"
         RPATH "")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "/home/stephano/OpenROAD/src/openroad")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/openroad" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/openroad")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/openroad")
    endif()
  endif()
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/stephano/OpenROAD/src/ifp/cmake_install.cmake")
  include("/home/stephano/OpenROAD/src/OpenDB/cmake_install.cmake")
  include("/home/stephano/OpenROAD/src/sta/cmake_install.cmake")
  include("/home/stephano/OpenROAD/src/dbSta/cmake_install.cmake")
  include("/home/stephano/OpenROAD/src/rsz/cmake_install.cmake")
  include("/home/stephano/OpenROAD/src/stt/cmake_install.cmake")
  include("/home/stephano/OpenROAD/src/replace/cmake_install.cmake")
  include("/home/stephano/OpenROAD/src/dpl/cmake_install.cmake")
  include("/home/stephano/OpenROAD/src/fin/cmake_install.cmake")
  include("/home/stephano/OpenROAD/src/ppl/cmake_install.cmake")
  include("/home/stephano/OpenROAD/src/rmp/cmake_install.cmake")
  include("/home/stephano/OpenROAD/src/cts/cmake_install.cmake")
  include("/home/stephano/OpenROAD/src/grt/cmake_install.cmake")
  include("/home/stephano/OpenROAD/src/tap/cmake_install.cmake")
  include("/home/stephano/OpenROAD/src/mpl/cmake_install.cmake")
  include("/home/stephano/OpenROAD/src/mpl2/cmake_install.cmake")
  include("/home/stephano/OpenROAD/src/rcx/cmake_install.cmake")
  include("/home/stephano/OpenROAD/src/psm/cmake_install.cmake")
  include("/home/stephano/OpenROAD/src/par/cmake_install.cmake")
  include("/home/stephano/OpenROAD/src/ant/cmake_install.cmake")
  include("/home/stephano/OpenROAD/src/gui/cmake_install.cmake")
  include("/home/stephano/OpenROAD/src/TritonRoute/cmake_install.cmake")
  include("/home/stephano/OpenROAD/src/utl/cmake_install.cmake")
  include("/home/stephano/OpenROAD/src/pdn/cmake_install.cmake")

endif()

