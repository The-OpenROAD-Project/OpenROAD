# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Config shim for the bazel-materialized Boost (modular BCR build).
# Compiled Boost objects live in the shared archive pool, so every
# component target is an interface over the merged include tree + pool.

include("${CMAKE_CURRENT_LIST_DIR}/../openroad_deps/deps-pool.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/../openroad_deps/deps-versions.cmake")

set(Boost_VERSION "${OPENROAD_DEPS_VERSION_boost}")
set(Boost_VERSION_STRING "${OPENROAD_DEPS_VERSION_boost}")
set(Boost_INCLUDE_DIRS "${_OR_DEPS}/include")

if(NOT TARGET Boost::headers)
  add_library(Boost::headers INTERFACE IMPORTED GLOBAL)
  set_target_properties(Boost::headers PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${_OR_DEPS}/include"
  )
  add_library(Boost::boost INTERFACE IMPORTED GLOBAL)
  set_target_properties(Boost::boost PROPERTIES
    INTERFACE_LINK_LIBRARIES Boost::headers
  )
endif()

set(Boost_LIBRARIES "")
foreach(_or_boost_comp IN LISTS Boost_FIND_COMPONENTS)
  if(NOT TARGET Boost::${_or_boost_comp})
    add_library(Boost::${_or_boost_comp} INTERFACE IMPORTED GLOBAL)
    set_target_properties(Boost::${_or_boost_comp} PROPERTIES
      INTERFACE_LINK_LIBRARIES "Boost::headers;openroad_deps::pool"
    )
  endif()
  set(Boost_${_or_boost_comp}_FOUND TRUE)
  list(APPEND Boost_LIBRARIES Boost::${_or_boost_comp})
endforeach()
unset(_or_boost_comp)
