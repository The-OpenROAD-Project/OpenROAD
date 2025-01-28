# Copyright 2023 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

include("openroad")

set(TEST_LIBS
  GTest::gtest
  GTest::gtest_main
  GTest::gmock
  odb
  mpl_lib
)

add_executable(TestSnapper TestSnapper.cpp)
target_link_libraries(TestSnapper ${TEST_LIBS})
gtest_discover_tests(TestSnapper WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(mpl_test mpl_test.cc)
target_link_libraries(mpl_test
    GTest::gtest
    GTest::gtest_main
    mpl_lib
)
gtest_discover_tests(mpl_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_dependencies(build_and_test
    mpl_test
    TestSnapper
)

