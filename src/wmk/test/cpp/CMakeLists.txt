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
