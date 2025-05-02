// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <tcl.h>

#include <string>

namespace ord {

// Call this inside of Tcl_Main.
void initOpenRoad(Tcl_Interp* interp,
                  const char* log_filename,
                  const char* metrics_filename,
                  bool batch_mode);
}  // namespace ord
