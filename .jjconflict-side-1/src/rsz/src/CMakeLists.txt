# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2025, The OpenROAD Authors

include("openroad")

swig_lib(NAME      rsz
         NAMESPACE rsz
         I_FILE    Resizer.i
         SWIG_INCLUDES ${OPENSTA_HOME}
         SCRIPTS   Resizer.tcl
)

add_library(rsz_lib
    BaseMove.cc
    BufferedNet.cc
    BufferMove.cc
    CloneMove.cc
    PreChecks.cc      
    RecoverPower.cc    
    RepairDesign.cc
    RepairHold.cc
    RepairSetup.cc
    Rebuffer.cc
    SizeDownMove.cc
    SizeUpMove.cc
    SplitLoadMove.cc
    SwapPinsMove.cc
    UnbufferMove.cc
    VTSwapMove.cc
    Resizer.cc
    OdbCallBack.cc
    ConcreteSwapArithModules.cc
)

target_sources(rsz
  PRIVATE
    Graphics.cc
    MakeResizer.cc
)

target_include_directories(rsz
  PUBLIC
    ../include
  PRIVATE
    # A side-effect of including OpenSTA swig files
    ${OPENSTA_HOME}
    ${OPENSTA_HOME}/include/sta
    .
)

target_include_directories(rsz_lib
  PUBLIC
    ../include
)

target_link_libraries(rsz_lib
  PUBLIC
    est_lib
    stt_lib
    odb
    OpenSTA
    dbSta_lib
    grt_lib
    utl_lib
)

target_link_libraries(rsz
  PUBLIC
    rsz_lib
    est
    stt
    odb
    OpenSTA
    dbSta
    grt
    gui
    utl_lib
)

messages(
  TARGET rsz
  OUTPUT_DIR ..
)
