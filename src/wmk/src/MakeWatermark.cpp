// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "wmk/MakeWatermark.h"

#include "tcl.h"
#include "utl/decode.h"

extern "C" {
extern int Wmk_Init(Tcl_Interp* interp);
}

namespace wmk {

// Tcl files encoded into strings by CMake.
extern const char* wmk_tcl_inits[];

void initWatermark(Tcl_Interp* tcl_interp)
{
  Wmk_Init(tcl_interp);
  utl::evalTclInit(tcl_interp, wmk::wmk_tcl_inits);
}

}  // namespace wmk
