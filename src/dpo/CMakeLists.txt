# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021-2025, The OpenROAD Authors

include("openroad")

swig_lib(NAME         dpo
         NAMESPACE    dpo
         I_FILE       src/Optdp.i
         SCRIPTS      src/Optdp.tcl
)

# https://github.com/The-OpenROAD-Project/OpenROAD/issues/1186
find_package(LEMON NAMES LEMON lemon REQUIRED)

target_sources(dpo
  PRIVATE
    src/Optdp.cpp
    src/MakeOptdp.cpp
    src/architecture.cxx
    src/color.cxx
    src/network.cxx
    src/utility.cxx
    src/legalize_shift.cxx
    src/detailed.cxx
    src/detailed_manager.cxx
    src/detailed_mis.cxx
    src/detailed_reorder.cxx
    src/detailed_global.cxx
    src/detailed_vertical.cxx
    src/detailed_displacement.cxx
    src/detailed_hpwl.cxx
    src/detailed_abu.cxx
    src/detailed_random.cxx
    src/detailed_orient.cxx
    src/journal.cxx
)

target_include_directories(dpo
  PUBLIC
    include
    ${LEMON_INCLUDE_DIRS}
)

target_link_libraries(dpo
  PRIVATE
    odb
    utl_lib
    dpl_lib
)

messages(
  TARGET dpo
)

################################################################

add_custom_target(optdp_tags etags -o TAGS
  ${OPTDP_SRC}
  ${OPTDP_HOME}/include/dpo/*.h
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

if (Python3_FOUND AND BUILD_PYTHON)
  swig_lib(NAME          dpo_py
           NAMESPACE     dpo
           LANGUAGE      python
           I_FILE        src/Optdp-py.i
           SWIG_INCLUDES ${PROJECT_SOURCE_DIR}/include/dpo
                         ${ODB_HOME}/src/swig/common
                         ${ODB_HOME}/src/swig/python
           SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/dpo_py.py
  )

  target_include_directories(dpo_py
    PUBLIC
      include
      include/dpo
  )

  target_link_libraries(dpo_py
    PUBLIC
      dpo
      odb
  )

endif()

add_subdirectory(test)
