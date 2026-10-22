// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

// The Bazel counterpart of the CMake-generated include/ord/Version.hh. The
// version is not compiled in: //bazel:stamped_binary.bzl writes it into the
// linked binary, so a new version invalidates no compile or link action.

namespace ord {
const char* stampedVersion();
}  // namespace ord

#define OPENROAD_VERSION ::ord::stampedVersion()
#define OPENROAD_GIT_DESCRIBE ""
