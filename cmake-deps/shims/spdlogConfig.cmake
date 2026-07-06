# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Config shim for bazel-materialized spdlog (header-only in the bazel
# graph, with external fmt; SPDLOG_FMT_EXTERNAL is injected globally by
# the compiler wrappers).

include("${CMAKE_CURRENT_LIST_DIR}/../openroad_deps/deps-pool.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/../openroad_deps/deps-versions.cmake")

set(spdlog_VERSION "${OPENROAD_DEPS_VERSION_spdlog}")

if(NOT TARGET spdlog::spdlog)
  add_library(spdlog::spdlog INTERFACE IMPORTED GLOBAL)
  set_target_properties(spdlog::spdlog PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${_OR_DEPS}/include"
    INTERFACE_LINK_LIBRARIES openroad_deps::pool
  )
  add_library(spdlog::spdlog_header_only ALIAS spdlog::spdlog)
endif()
