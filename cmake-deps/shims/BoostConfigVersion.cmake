# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# AnyNewerVersion semantics for the Boost config shim.

include("${CMAKE_CURRENT_LIST_DIR}/../openroad_deps/deps-versions.cmake")

set(PACKAGE_VERSION "${OPENROAD_DEPS_VERSION_boost}")
if(PACKAGE_VERSION VERSION_LESS PACKAGE_FIND_VERSION)
  set(PACKAGE_VERSION_COMPATIBLE FALSE)
else()
  set(PACKAGE_VERSION_COMPATIBLE TRUE)
  if(PACKAGE_FIND_VERSION STREQUAL PACKAGE_VERSION)
    set(PACKAGE_VERSION_EXACT TRUE)
  endif()
endif()
