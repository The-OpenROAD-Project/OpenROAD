# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Config shim for bazel-materialized OR-Tools (with scip, highs, glpk,
# soplex, bliss, protobuf and abseil in the archive pool).

include("${CMAKE_CURRENT_LIST_DIR}/../openroad_deps/deps-pool.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/../openroad_deps/deps-versions.cmake")

set(ortools_VERSION "${OPENROAD_DEPS_VERSION_or_tools}")

if(NOT TARGET ortools::ortools)
  add_library(ortools::ortools INTERFACE IMPORTED GLOBAL)
  set_target_properties(ortools::ortools PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${_OR_DEPS}/include"
    INTERFACE_LINK_LIBRARIES openroad_deps::pool
  )
endif()
