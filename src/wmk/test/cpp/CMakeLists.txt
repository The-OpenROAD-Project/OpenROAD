# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

include("openroad")

# Every test links the module library, so it exercises the same objects the
# tool ships, built with the same flags.
function(wmk_gtest name)
  add_executable(${name} ${name}.cpp)
  target_include_directories(${name} PRIVATE ../../src)
  target_link_libraries(${name} PRIVATE GTest::gtest_main wmk odb ${ARGN})
  gtest_discover_tests(${name} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  add_dependencies(build_and_test ${name})
endfunction()

wmk_gtest(TestHmacSha256)
wmk_gtest(TestMarks)
wmk_gtest(TestClaims Boost::headers)
wmk_gtest(TestTiming dbSta_lib OpenSTA)
wmk_gtest(TestClaimFile)
wmk_gtest(TestRoutingStat)
wmk_gtest(TestVerifyResult Boost::headers)

# These read a technology from the shared test data, so they run from the
# source root like the fixture expects.
foreach(name TestDriverHeadroom TestClockTree)
  add_executable(${name} ${name}.cpp)
  target_include_directories(${name} PRIVATE ../../src)
  target_link_libraries(${name} PRIVATE GTest::gtest_main tst wmk odb dbSta_lib OpenSTA)
  gtest_discover_tests(${name} WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
  add_dependencies(build_and_test ${name})
endforeach()
