# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2018-2025, The OpenROAD Authors

include("openroad")
find_package(TCL)
find_package(Boost)

swig_lib(NAME         dpl
         NAMESPACE    dpl
         I_FILE       src/Opendp.i
         SCRIPTS      src/Opendp.tcl
)

add_library(dpl_lib
  src/Opendp.cpp
  src/dbToOpendp.cpp
  src/Grid.cpp
  src/CheckPlacement.cpp
  src/Objects.cpp
  src/Padding.cpp
  src/Place.cpp
  src/FillerPlacement.cpp
  src/DecapPlacement.cpp
  src/OptMirror.cpp
  src/PlacementDRC.cpp
)

target_link_libraries(dpl_lib
  PUBLIC
    odb
  PRIVATE
    utl_lib
)

target_include_directories(dpl_lib
  PUBLIC
    include
)

target_sources(dpl
  PRIVATE
    src/MakeOpendp.cpp
    src/Graphics.cpp
    src/DplObserver.h
)

target_include_directories(dpl
  PUBLIC
    include
  PRIVATE
    src
)

target_link_libraries(dpl
  PRIVATE
    ${TCL_LIBRARY}
    dpl_lib
    gui
)

messages(
  TARGET dpl
)

################################################################

add_custom_target(opendp_tags etags -o TAGS
  ${OPENDP_SRC}
  ${OPENDP_HOME}/include/dpl/*.h
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

if (Python3_FOUND AND BUILD_PYTHON)
  swig_lib(NAME          dpl_py
           NAMESPACE     dpl
           LANGUAGE      python
           I_FILE        src/Opendp-py.i
           SWIG_INCLUDES ${PROJECT_SOURCE_DIR}/include/dpl
                         ${ODB_HOME}/src/swig/common
                         ${ODB_HOME}/src/swig/python
           SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/dpl_py.py
  )

  target_link_libraries(dpl_py
    PUBLIC
      ${Boost_LIBRARIES}
      dpl_lib
      gui
      odb
  )

endif()

if(ENABLE_TESTS)
  add_subdirectory(test)
endif()
