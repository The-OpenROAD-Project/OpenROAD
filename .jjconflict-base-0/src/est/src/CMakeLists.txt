# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2025, The OpenROAD Authors

include("openroad")

swig_lib(NAME      est
         NAMESPACE est
         I_FILE    EstimateParasitics.i
         SWIG_INCLUDES ${OPENSTA_HOME}
         SCRIPTS   EstimateParasitics.tcl
)

add_library(est_lib
    EstimateParasitics.cpp
    EstimateParasiticsCallBack.cpp
    MakeWireParasitics.cpp
    OdbCallBack.cpp
    SteinerTree.cpp
)

target_sources(est
  PRIVATE
    MakeEstimateParasitics.cpp
    SteinerRenderer.cpp
)

target_include_directories(est
  PUBLIC
    ../include
  PRIVATE
    # A side-effect of including OpenSTA swig files
    ${OPENSTA_HOME}
    ${OPENSTA_HOME}/include/sta
    .
)

target_include_directories(est_lib
  PUBLIC
    ../include
)

target_link_libraries(est_lib
  PUBLIC
    stt_lib
    odb
    OpenSTA
    dbSta_lib
    grt_lib
    utl_lib
)

target_link_libraries(est
  PUBLIC
    est_lib
    stt
    odb
    OpenSTA
    dbSta
    grt
    utl_lib
)

messages(
  TARGET est
  OUTPUT_DIR ..
)
