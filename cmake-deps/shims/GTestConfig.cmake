# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Config shim for bazel-materialized GoogleTest. The BCR build folds gmock
# into libgtest.a. These archives are deliberately not in the shared pool:
# gtest_main carries main().

include("${CMAKE_CURRENT_LIST_DIR}/../openroad_deps/deps-pool.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/../openroad_deps/deps-versions.cmake")

set(GTest_VERSION "${OPENROAD_DEPS_VERSION_googletest}")

if(NOT TARGET GTest::gtest)
  add_library(GTest::gtest STATIC IMPORTED GLOBAL)
  set_target_properties(GTest::gtest PROPERTIES
    IMPORTED_LOCATION "${_OR_DEPS}/lib/pool/googletest+/libgtest.a"
    INTERFACE_INCLUDE_DIRECTORIES "${_OR_DEPS}/include"
    INTERFACE_LINK_LIBRARIES openroad_deps::pool
  )
  add_library(GTest::gtest_main STATIC IMPORTED GLOBAL)
  set_target_properties(GTest::gtest_main PROPERTIES
    IMPORTED_LOCATION "${_OR_DEPS}/lib/pool/googletest+/libgtest_main.a"
    INTERFACE_LINK_LIBRARIES GTest::gtest
  )
  add_library(GTest::gmock ALIAS GTest::gtest)
  add_library(GTest::gmock_main ALIAS GTest::gtest_main)
  add_library(GTest::GTest ALIAS GTest::gtest)
  add_library(GTest::Main ALIAS GTest::gtest_main)
endif()
