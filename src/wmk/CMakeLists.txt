# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

include("openroad")

swig_lib(NAME      wmk
         NAMESPACE wmk
         I_FILE    src/watermark.i
         SCRIPTS   src/watermark.tcl
)

target_sources(wmk
  PRIVATE
    src/Watermark.cpp
    src/Wirelength.cpp
    src/PlacementWatermark.cpp
    src/CtsWatermark.cpp
    src/RoutingStat.cpp
    src/ClockTree.cpp
    src/Verify.cpp
    src/Claims.cpp
    src/MakeWatermark.cpp
    src/HmacSha256.cpp
    src/Keys.cpp
)

target_include_directories(wmk
  PUBLIC
    include
  PRIVATE
    src
)

# dbSta and OpenSTA are needed to screen candidates on slack, so the watermark
# does not land on timing-critical cells; dpl_lib re-legalizes after a swap.
target_link_libraries(wmk
  PRIVATE
    ${TCL_LIBRARY}
    odb
    utl_lib
    OpenSTA
    dbSta_lib
    dpl_lib
    est_lib
)

messages(
  TARGET wmk
)

################################################################

if (Python3_FOUND AND BUILD_PYTHON)
  swig_lib(NAME          wmk_py
           NAMESPACE     wmk
           LANGUAGE      python
           I_FILE        src/watermark-py.i
           SWIG_INCLUDES ${ODB_HOME}/src/swig/common
                         ${ODB_HOME}/src/swig/python
           SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/wmk_py.py
  )

  target_include_directories(wmk_py
    PRIVATE
      src
  )

  # odb is listed explicitly: wmk links it privately, so the public header's
  # odb/db.h include is not satisfied through wmk alone.
  target_link_libraries(wmk_py
    PUBLIC
      wmk
      odb
  )

endif()

if(ENABLE_TESTS)
  add_subdirectory(test)
endif()
