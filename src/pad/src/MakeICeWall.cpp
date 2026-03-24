// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "pad/MakeICeWall.h"

#include "tcl.h"
#include "utl/decode.h"

extern "C" {
extern int Pad_Init(Tcl_Interp* interp);
}

namespace pad {

extern const char* pad_tcl_inits[];

void initICeWall(Tcl_Interp* tcl_interp)
{
  // Define swig TCL commands.
  Pad_Init(tcl_interp);
  // Eval encoded sta TCL sources.
  utl::evalTclInit(tcl_interp, pad::pad_tcl_inits);
}

}  // namespace pad
