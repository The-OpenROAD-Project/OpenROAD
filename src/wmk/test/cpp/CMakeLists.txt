# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors

include("openroad")

# HmacSha256 is self-contained -- no odb, no logger -- so the test compiles it
# directly rather than pulling in the whole module.
add_executable(TestHmacSha256
  TestHmacSha256.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../../src/HmacSha256.cpp
)

target_include_directories(TestHmacSha256
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
)

target_link_libraries(TestHmacSha256
  GTest::gtest
  GTest::gtest_main
)

gtest_discover_tests(TestHmacSha256
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_dependencies(build_and_test TestHmacSha256)

add_executable(TestClaims TestClaims.cpp ../../src/Claims.cpp)
target_include_directories(TestClaims PRIVATE ../../src)
target_link_libraries(TestClaims PRIVATE GTest::gtest_main Boost::headers)
gtest_discover_tests(TestClaims WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_dependencies(build_and_test TestClaims)

add_executable(TestTiming TestTiming.cpp ../../src/Timing.cpp)
target_include_directories(TestTiming PRIVATE ../../src)
target_link_libraries(TestTiming PRIVATE GTest::gtest_main dbSta_lib OpenSTA)
gtest_discover_tests(TestTiming WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_dependencies(build_and_test TestTiming)

add_executable(TestDriverHeadroom TestDriverHeadroom.cpp ../../src/Timing.cpp)
target_include_directories(TestDriverHeadroom PRIVATE ../../src)
target_link_libraries(TestDriverHeadroom PRIVATE GTest::gtest_main tst dbSta_lib OpenSTA)
gtest_discover_tests(TestDriverHeadroom WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
add_dependencies(build_and_test TestDriverHeadroom)

add_executable(TestClockTree TestClockTree.cpp ../../src/ClockTree.cpp)
target_include_directories(TestClockTree PRIVATE ../../src)
target_link_libraries(TestClockTree PRIVATE GTest::gtest_main tst dbSta_lib OpenSTA)
gtest_discover_tests(TestClockTree WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
add_dependencies(build_and_test TestClockTree)

add_executable(TestClaimFile TestClaimFile.cpp ../../src/ClaimFile.cpp)
target_include_directories(TestClaimFile PRIVATE ../../src)
target_link_libraries(TestClaimFile PRIVATE GTest::gtest_main)
gtest_discover_tests(TestClaimFile WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_dependencies(build_and_test TestClaimFile)

add_executable(TestRoutingStat TestRoutingStat.cpp)
target_include_directories(TestRoutingStat PRIVATE ../../src)
target_link_libraries(TestRoutingStat PRIVATE GTest::gtest_main wmk odb)
gtest_discover_tests(TestRoutingStat WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_dependencies(build_and_test TestRoutingStat)
