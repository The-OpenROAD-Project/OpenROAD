# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2020-2025, The OpenROAD Authors

include("openroad")

add_library(rcx_lib
  ext.cpp
  extBench.cpp
  extBenchDB.cpp
  extCC.cpp
  extCoords.cpp
  extFlow.cpp
  extRCmodel.cpp
  extSpef.cpp
  extSpefIn.cpp
  extmain.cpp
  extmeasure.cpp
  extDebugPrint.cpp
  extstats.cpp
  extprocess.cpp
  netRC.cpp
  extmeasure_res.cpp
  name.cpp
  grids.cpp
  gs.cpp
  dbUtil.cpp
  process_ext.cpp
  extRCmodel_process.cpp
  extRCmodel_solver.cpp
  extSolverGen.cpp
  extModelGen.cpp
  extFlow_v2.cpp
  extmeasure_diag.cpp
  extSegment.cpp
  findsegments.cpp
  extmeasure_couple.cpp
  extmeasure_dist.cpp
  extmeasure_flow.cpp
  extmeasure_print.cpp
  extmeasure_calc.cpp
  extmeasure_rc_flow.cpp
  extRCmodel_process.cpp
  extRCmodel_solver.cpp
  extDefPatterns.cpp
  extDefPatternsVia.cpp
  extPatterns.cpp
  extRulesVia.cpp
  extmeasure_print.cpp
  extmeasure_res_v2.cpp
  extmain_v2.cpp
  extmeasure_diag_opt.cpp
  parse.cpp
  util.cpp
)

target_include_directories(rcx_lib
  PUBLIC
    ../include
    ../include/rcx
)

target_link_libraries(rcx_lib
  PUBLIC
    odb
    utl_lib
)

swig_lib(NAME      rcx
         NAMESPACE rcx
         I_FILE    ext.i
         SCRIPTS   OpenRCX.tcl OpenRCX_process.tcl
)

target_sources(rcx
  PRIVATE
    MakeOpenRCX.cpp
)

target_include_directories(rcx
  PUBLIC
    ../include
    ../include/rcx
)

target_link_libraries(rcx
  PUBLIC
    rcx_lib
    utl_lib
)

messages(
  TARGET rcx
  OUTPUT_DIR ..
)

############################################################
# Unit testing
############################################################
if(ENABLE_TESTS)
  enable_testing()

  add_executable(rcxUnitTest
    ${PROJECT_SOURCE_DIR}/test/ext2dBoxTest.cpp
  )

  target_include_directories(rcxUnitTest
    PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${OPENROAD_HOME}/include
  )

  target_link_libraries(rcxUnitTest
    rcx_lib
  )

  # Use the shared library if found.  We need to pass this info to
  # the code to select the corresponding include.  Using the shared
  # library speeds up compilation.
  if (Boost_unit_test_framework_FOUND)
    message(STATUS "Boost unit_test_framework library found")
    target_link_libraries(rcxUnitTest
      Boost::unit_test_framework
    )
    target_compile_definitions(rcxUnitTest
      PRIVATE
      HAS_BOOST_UNIT_TEST_LIBRARY
    )
  endif()

  add_test(NAME rcxUnitTest COMMAND rcxUnitTest)
  add_dependencies(build_and_test rcxUnitTest)
endif()

if (Python3_FOUND AND BUILD_PYTHON)
  swig_lib(NAME          rcx_py
           NAMESPACE     rcx
           LANGUAGE      python
           I_FILE        ext-py.i
           SWIG_INCLUDES ${PROJECT_SOURCE_DIR}/../include
           SCRIPTS       ${CMAKE_CURRENT_BINARY_DIR}/rcx_py.py
  )

  target_include_directories(rcx_py
    PUBLIC
      ../include
  )

  target_link_libraries(rcx_py
    PUBLIC
      rcx_lib
  )

endif()
