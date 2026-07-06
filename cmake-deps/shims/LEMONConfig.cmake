# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026, The OpenROAD Authors
#
# Config shim for bazel-materialized COIN-OR LEMON. OpenROAD only consumes
# ${LEMON_INCLUDE_DIRS}; the compiled objects are in the archive pool.

include("${CMAKE_CURRENT_LIST_DIR}/../openroad_deps/deps-pool.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/../openroad_deps/deps-versions.cmake")

set(LEMON_VERSION "${OPENROAD_DEPS_VERSION_coin_or_lemon}")
set(LEMON_INCLUDE_DIR "${_OR_DEPS}/include")
set(LEMON_INCLUDE_DIRS "${_OR_DEPS}/include")
set(LEMON_LIBRARIES openroad_deps::pool)
