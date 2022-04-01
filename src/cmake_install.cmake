# Install script for directory: /home/kevinchen/OpenROAD/src

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
    set(CMAKE_INSTALL_CONFIG_NAME "RELEASE")
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
  set(CMAKE_INSTALL_SO_NO_EXE "0")
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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE EXECUTABLE FILES "/home/kevinchen/OpenROAD/src/openroad")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/openroad" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/openroad")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/opt/rh/devtoolset-8/root/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/openroad")
    endif()
  endif()
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/kevinchen/OpenROAD/src/ifp/cmake_install.cmake")
  include("/home/kevinchen/OpenROAD/src/odb/cmake_install.cmake")
  include("/home/kevinchen/OpenROAD/src/sta/cmake_install.cmake")
  include("/home/kevinchen/OpenROAD/src/dbSta/cmake_install.cmake")
  include("/home/kevinchen/OpenROAD/src/rsz/cmake_install.cmake")
  include("/home/kevinchen/OpenROAD/src/stt/cmake_install.cmake")
  include("/home/kevinchen/OpenROAD/src/gpl/cmake_install.cmake")
  include("/home/kevinchen/OpenROAD/src/dpl/cmake_install.cmake")
  include("/home/kevinchen/OpenROAD/src/dpo/cmake_install.cmake")
  include("/home/kevinchen/OpenROAD/src/fin/cmake_install.cmake")
  include("/home/kevinchen/OpenROAD/src/ppl/cmake_install.cmake")
  include("/home/kevinchen/OpenROAD/src/rmp/cmake_install.cmake")
  include("/home/kevinchen/OpenROAD/src/cts/cmake_install.cmake")
  include("/home/kevinchen/OpenROAD/src/grt/cmake_install.cmake")
  include("/home/kevinchen/OpenROAD/src/tap/cmake_install.cmake")
  include("/home/kevinchen/OpenROAD/src/mpl/cmake_install.cmake")
  include("/home/kevinchen/OpenROAD/src/mpl2/cmake_install.cmake")
  include("/home/kevinchen/OpenROAD/src/rcx/cmake_install.cmake")
  include("/home/kevinchen/OpenROAD/src/psm/cmake_install.cmake")
  include("/home/kevinchen/OpenROAD/src/par/cmake_install.cmake")
  include("/home/kevinchen/OpenROAD/src/ant/cmake_install.cmake")
  include("/home/kevinchen/OpenROAD/src/gui/cmake_install.cmake")
  include("/home/kevinchen/OpenROAD/src/drt/cmake_install.cmake")
  include("/home/kevinchen/OpenROAD/src/utl/cmake_install.cmake")
  include("/home/kevinchen/OpenROAD/src/dst/cmake_install.cmake")
  include("/home/kevinchen/OpenROAD/src/pdn/cmake_install.cmake")

endif()

