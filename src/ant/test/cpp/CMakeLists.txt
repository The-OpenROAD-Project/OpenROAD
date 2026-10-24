# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

find_package(OpenMP REQUIRED)

foreach(TEST_NAME
    TestAntennaDesigns
    TestAntennaReport
    TestAntennaRules
    TestAntennaSynthetic
)
  add_executable(${TEST_NAME} ${TEST_NAME}.cpp)
  target_include_directories(${TEST_NAME}
    PRIVATE
      ${PROJECT_SOURCE_DIR}/src/ant/src
  )
  target_link_libraries(${TEST_NAME}
    GTest::gtest
    GTest::gtest_main
    ant_lib
    tst_base
    tst_physical
    db
    utl_lib
    OpenMP::OpenMP_CXX
  )
  gtest_discover_tests(${TEST_NAME}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
  )
  add_dependencies(build_and_test ${TEST_NAME})
endforeach()
