// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Boost.JSON is header-only and compiled once here for the CMake build.  The
// Bazel build gets the same symbols from the @boost.json library.

#ifndef BAZEL_BUILD
#include <boost/json/src.hpp>
#endif
