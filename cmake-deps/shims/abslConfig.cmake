# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Config shim for bazel-materialized Abseil. Only the targets OpenROAD's
# CMake files reference are defined; all are interfaces over the merged
# include tree + archive pool.

include("${CMAKE_CURRENT_LIST_DIR}/../openroad_deps/deps-pool.cmake")

foreach(_or_absl_target city hash strings synchronization base time span)
  if(NOT TARGET absl::${_or_absl_target})
    add_library(absl::${_or_absl_target} INTERFACE IMPORTED GLOBAL)
    set_target_properties(absl::${_or_absl_target} PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${_OR_DEPS}/include"
      INTERFACE_LINK_LIBRARIES openroad_deps::pool
    )
  endif()
endforeach()
unset(_or_absl_target)
