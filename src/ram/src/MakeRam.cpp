// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "ram/MakeRam.h"

#include "tcl.h"
#include "utl/decode.h"

extern "C" {
extern int Ram_Init(Tcl_Interp* interp);
}

namespace ram {

// Tcl files encoded into strings.
extern const char* ram_tcl_inits[];

void initRamGen(Tcl_Interp* tcl_interp)
{
  // Define swig TCL commands.
  Ram_Init(tcl_interp);
  // Eval encoded sta TCL sources.
  utl::evalTclInit(tcl_interp, ram::ram_tcl_inits);
}

}  // namespace ram
