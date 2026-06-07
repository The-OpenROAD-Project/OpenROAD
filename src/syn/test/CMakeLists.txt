# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

include("openroad")

or_integration_tests(
  "syn"
)

set(_syn_test_libs
  GTest::gtest
  GTest::gtest_main
)

foreach(_t ir_test graph_test parse_test)
  add_executable(${_t} ${_t}.cc)
  target_link_libraries(${_t} ${_syn_test_libs} syn_ir)
  gtest_discover_tests(${_t} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  add_dependencies(build_and_test ${_t})
endforeach()

foreach(_t opt_test bitblast_test)
  add_executable(${_t} ${_t}.cc)
  target_link_libraries(${_t} ${_syn_test_libs} syn_flow syn_lib syn_ir)
  gtest_discover_tests(${_t} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  add_dependencies(build_and_test ${_t})
endforeach()

add_executable(liveness_test liveness_test.cc)
# No GTest::gtest_main: ABC ships its own main() that wins over gtest_main.
target_link_libraries(liveness_test GTest::gtest syn_flow syn_lib syn_ir utl_lib)
gtest_discover_tests(liveness_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_dependencies(build_and_test liveness_test)

foreach(_t abc_test sm_test cm_test)
  add_executable(${_t} ${_t}.cc)
  target_link_libraries(${_t} ${_syn_test_libs} syn_flow syn_lib syn_ir tst)
  gtest_discover_tests(${_t} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  add_dependencies(build_and_test ${_t})
endforeach()

add_executable(elab_test elab_test.cc)
target_link_libraries(elab_test ${_syn_test_libs} syn_elab syn_ir)
gtest_discover_tests(elab_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_dependencies(build_and_test elab_test)
