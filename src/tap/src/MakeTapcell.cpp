// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "src/tap/include/tap/MakeTapcell.h"

#include "src/tap/include/tap/tapcell.h"
#include "src/utl/include/utl/decode.h"
#include "tcl.h"

extern "C" {
extern int Tap_Init(Tcl_Interp* interp);
}

namespace tap {
// Tcl files encoded into strings.
extern const char* tap_tcl_inits[];

void initTapcell(Tcl_Interp* tcl_interp)
{
  Tap_Init(tcl_interp);
  // Eval encoded sta TCL sources.
  utl::evalTclInit(tcl_interp, tap::tap_tcl_inits);
}

}  // namespace tap
