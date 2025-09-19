# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023-2025, The OpenROAD Authors

include("openroad")
project("dft")

swig_lib(NAME dft
         NAMESPACE dft
         I_FILE src/dft.i
         SCRIPTS src/dft.tcl
)

#################################################################
#
# Source files.
#
################################################################
target_sources(dft
  PRIVATE
  # Keep sorted
    src/Dft.cpp
    src/MakeDft.cpp
)


target_include_directories(dft
  PUBLIC
    include
)

target_link_libraries(dft
  PUBLIC
    odb
    dbSta
    OpenSTA
)

target_link_libraries(dft
  PRIVATE
    dft_cells_lib
    dft_architect_lib
    dft_config_lib
    dft_replace_lib
    dft_clock_domain_lib
    dft_utils_lib
    dft_stitch_lib
)


if (Python3_FOUND AND BUILD_PYTHON)
  swig_lib(NAME          dft_py
           NAMESPACE     dft
           LANGUAGE      python
           I_FILE        src/DftGen-py.i
           SWIG_INCLUDES ${ODB_HOME}/src/swig/common
                         ${ODB_HOME}/src/swig/python
                         ${ODB_HOME}/include
                         ${PROJECT_SOURCE_DIR}/include
           SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/dft_py.py
  )

  target_include_directories(dft_py
    PUBLIC
      ../include
  )
  
  target_link_libraries(dft_py
    PUBLIC
      dft
    )

endif()

# Keep sorted
add_subdirectory(src/architect)
add_subdirectory(src/cells)
add_subdirectory(src/clock_domain)
add_subdirectory(src/config)
add_subdirectory(src/replace)
add_subdirectory(src/stitch)
add_subdirectory(src/utils)
if(ENABLE_TESTS)
  add_subdirectory(test)
endif()

messages(
  TARGET dft
)
