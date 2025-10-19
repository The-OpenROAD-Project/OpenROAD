# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2025, The OpenROAD Authors

include("openroad")



find_package(ortools REQUIRED)

add_library(mpl_lib
  src/rtl_mp.cpp
  src/hier_rtlmp.cpp
  src/object.cpp
  src/SimulatedAnnealingCore.cpp
  src/SACoreHardMacro.cpp
  src/SACoreSoftMacro.cpp
  src/clusterEngine.cpp
  src/shapes.cpp
)

target_include_directories(mpl_lib
  PUBLIC
    include
)

target_link_libraries(mpl_lib
    utl_lib
    dbSta_lib
    OpenSTA
    odb
    ortools::ortools
    dl
    par_lib
)

swig_lib(NAME      mpl
         NAMESPACE mpl
         I_FILE    src/mpl.i
         SCRIPTS   src/mpl.tcl
)

target_sources(mpl
  PRIVATE
    src/MakeMacroPlacer.cpp
    src/graphics.cpp
)

messages(
  TARGET mpl
)

target_include_directories(mpl
  PUBLIC
    include
  PRIVATE
    src
)

target_link_libraries(mpl
    mpl_lib
    gui
)

if(ENABLE_TESTS)
  add_subdirectory(test)
endif()
