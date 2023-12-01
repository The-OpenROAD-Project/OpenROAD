# Copyright 2023 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

include("openroad")

add_executable(cts_unittest cts_unittest.cc)
target_include_directories(cts_unittest
  PUBLIC
    ${OPENROAD_HOME}
)

target_link_libraries(cts_unittest
    gtest
    gtest_main
    cts_lib
    utl_lib
    dbSta_lib
    OpenSTA
    odb
)

gtest_discover_tests(cts_unittest
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_dependencies(build_and_test cts_unittest)

