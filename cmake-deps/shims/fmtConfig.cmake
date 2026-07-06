# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Config shim for bazel-materialized fmt.

include("${CMAKE_CURRENT_LIST_DIR}/../openroad_deps/deps-pool.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/../openroad_deps/deps-versions.cmake")

set(fmt_VERSION "${OPENROAD_DEPS_VERSION_fmt}")

if(NOT TARGET fmt::fmt)
  add_library(fmt::fmt INTERFACE IMPORTED GLOBAL)
  set_target_properties(fmt::fmt PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${_OR_DEPS}/include"
    INTERFACE_LINK_LIBRARIES openroad_deps::pool
  )
  add_library(fmt::fmt-header-only INTERFACE IMPORTED GLOBAL)
  set_target_properties(fmt::fmt-header-only PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${_OR_DEPS}/include"
    INTERFACE_COMPILE_DEFINITIONS FMT_HEADER_ONLY=1
  )
endif()
