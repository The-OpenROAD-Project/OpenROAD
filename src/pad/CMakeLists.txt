# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2025, The OpenROAD Authors

include("openroad")

swig_lib(NAME      pad
         NAMESPACE pad
         I_FILE    src/pad.i
         SWIG_INCLUDES ${ODB_HOME}/src/swig/common
                       ${ODB_HOME}/src/swig/tcl
                       ${ODB_HOME}/include
         SCRIPTS   src/pad.tcl
)

target_sources(pad
  PRIVATE
    src/MakeICeWall.cpp
    src/ICeWall.cpp
    src/RDLRouter.cpp
    src/RDLRoute.cpp
    src/RDLGui.cpp
    src/Utilities.cpp
)

target_include_directories(pad
  PUBLIC
    include
)

target_link_libraries(pad
  PRIVATE
    utl_lib
    odb
    gui
    Boost::boost
)

messages(
  TARGET pad
)

add_subdirectory(test)
