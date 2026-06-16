// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include "tcl.h"

namespace in_bazel {
// Set up tcl enviromment and provide all the libraries.
int SetupTclEnvironment(Tcl_Interp* interp);
}  // namespace in_bazel
