// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include "tcl.h"

namespace ord {
// Initialiize tcl readline, applying various ways to initialize.
int SetupTclReadlineLibrary(Tcl_Interp* interp);
}  // namespace ord
