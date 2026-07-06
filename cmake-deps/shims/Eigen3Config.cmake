# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Config shim for bazel-materialized Eigen (header-only).

include("${CMAKE_CURRENT_LIST_DIR}/../openroad_deps/deps-pool.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/../openroad_deps/deps-versions.cmake")

set(Eigen3_VERSION "${OPENROAD_DEPS_VERSION_eigen}")
set(EIGEN3_INCLUDE_DIR "${_OR_DEPS}/include")
set(EIGEN3_INCLUDE_DIRS "${_OR_DEPS}/include")

if(NOT TARGET Eigen3::Eigen)
  add_library(Eigen3::Eigen INTERFACE IMPORTED GLOBAL)
  set_target_properties(Eigen3::Eigen PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${_OR_DEPS}/include"
  )
endif()
