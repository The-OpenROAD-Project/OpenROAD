# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Config shim for bazel-materialized yaml-cpp. Upstream's config defines
# the plain `yaml-cpp` target (odb/3dblox links it) plus the namespaced
# alias.

include("${CMAKE_CURRENT_LIST_DIR}/../openroad_deps/deps-pool.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/../openroad_deps/deps-versions.cmake")

set(yaml-cpp_VERSION "${OPENROAD_DEPS_VERSION_yaml_cpp}")

if(NOT TARGET yaml-cpp)
  add_library(yaml-cpp INTERFACE IMPORTED GLOBAL)
  set_target_properties(yaml-cpp PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${_OR_DEPS}/include"
    INTERFACE_LINK_LIBRARIES openroad_deps::pool
  )
  add_library(yaml-cpp::yaml-cpp ALIAS yaml-cpp)
endif()
