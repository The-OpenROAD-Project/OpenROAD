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
    BufferedNet.cc
    PreChecks.cc
    RecoverPower.cc
    RepairDesign.cc
    RepairHold.cc
    Rebuffer.cc
    policy/SetupLegacyMtPolicy.cc
    policy/SetupLegacyBase.cc
    MoveCommitter.cc
    MoveTracker.cc
    policy/OptimizationPolicy.cc
    policy/OptimizationPolicy.hh
    Optimizer.cc
    OptimizerTypes.cc
    policy/MeasuredVtSwapPolicy.cc
    DelayEstimator.cc
    DelayEstimatorReporter.cc
    policy/SetupCritVtSwapPolicy.cc
    policy/SetupDirectionalPolicy.cc
    policy/SetupLastGaspPolicy.cc
    policy/SetupLegacyPolicy.cc
    policy/SetupTnsPolicy.cc
    policy/SetupWnsPolicy.cc
    policy/SetupMt1Policy.cc
    policy/SetupReroutePolicy.cc
    Resizer.cc
    OdbCallBack.cc
    ConcreteSwapArithModules.cc
    RepairTargetCollector.cc

    move/BufferCandidate.cc
    move/BufferGenerator.cc
    move/CloneCandidate.cc
    move/CloneGenerator.cc
    move/MeasuredVtSwapCandidate.cc
    move/MeasuredVtSwapGenerator.cc
    move/MoveCandidate.cc
    move/MoveGenerator.cc
    move/RerouteCandidate.cc
    move/RerouteGenerator.cc
    move/SizeDownCandidate.cc
    move/SizeDownGenerator.cc
    move/SizeUpCandidate.cc
    move/SizeUpGenerator.cc
    move/SizeUpMatchCandidate.cc
    move/SizeUpMatchGenerator.cc
    move/SizeUpMtCandidate.cc
    move/SizeUpMtGenerator.cc
    move/SplitLoadCandidate.cc
    move/SplitLoadGenerator.cc
    move/SwapPinsCandidate.cc
    move/SwapPinsGenerator.cc
    move/UnbufferCandidate.cc
    move/UnbufferGenerator.cc
    move/VtSwapCandidate.cc
    move/VtSwapGenerator.cc
    move/VtSwapMtCandidate.cc
    move/VtSwapMtGenerator.cc
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
    policy
)

target_include_directories(rsz_lib
  PUBLIC
    ../include
  PRIVATE
    .
    move
    policy
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

if (Python3_FOUND AND BUILD_PYTHON)
  swig_lib(NAME          rsz_py
           NAMESPACE     rsz
           LANGUAGE      python
           I_FILE        Resizer-py.i
           SWIG_INCLUDES ${OPENSTA_HOME}
           SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/rsz_py.py
  )

  target_include_directories(rsz_py
    PUBLIC
      ../include
    PRIVATE
      ${OPENSTA_HOME}
      ${OPENSTA_HOME}/include/sta
  )

  target_link_libraries(rsz_py
    PUBLIC
      rsz_lib
  )
endif()
