###########################################################################
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023-2025, The OpenROAD Authors

include("openroad")

swig_lib(NAME      ram
         NAMESPACE ram
         I_FILE    src/ram.i
         SCRIPTS   src/ram.tcl
)

target_sources(ram
  PRIVATE
    src/ram.cpp
    src/layout.cpp
    src/MakeRam.cpp
)

target_include_directories(ram
  PUBLIC
    include
)

target_link_libraries(ram
  PRIVATE
    odb
    dbSta
    OpenSTA
    pdn
    ppl
    dpl
    grt
    drt
)

messages(
  TARGET ram
)

if (Python3_FOUND AND BUILD_PYTHON)
  swig_lib(NAME          ram_py
           NAMESPACE     ram
           LANGUAGE      python
           I_FILE        src/ram-py.i
           SWIG_INCLUDES ${PROJECT_SOURCE_DIR}/include/ram
           SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/ram_py.py
  )

  target_link_libraries(ram_py
    PUBLIC
      ram
      odb
      pdn
  )

endif()

add_subdirectory(test)
