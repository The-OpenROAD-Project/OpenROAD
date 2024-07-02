# Copyright 2023 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

include("openroad")

set(TEST_LIBS
  gtest
  gmock
  gtest_main
  odb
  mpl2_lib
)

add_executable(TestSnapper TestSnapper.cpp)
target_link_libraries(TestSnapper ${TEST_LIBS})
gtest_discover_tests(TestSnapper WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(mpl2_test mpl2_test.cc)
target_link_libraries(mpl2_test 
    gtest 
    gtest_main
    mpl2_lib
)
gtest_discover_tests(mpl2_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_dependencies(build_and_test
    mpl2_test
    TestSnapper
)

