// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#pragma once

extern "C" {
struct Tcl_Interp;
}

namespace ord {

class OpenRoad;

void initSurrogate(Tcl_Interp* interp, OpenRoad* openroad);

}  // namespace ord

