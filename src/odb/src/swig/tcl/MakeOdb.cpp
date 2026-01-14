// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "odb/MakeOdb.h"

#include "tcl.h"
#include "utl/decode.h"

extern "C" {
extern int Odbtcl_Init(Tcl_Interp* interp);
}

namespace odb {

// Tcl files encoded into strings.
extern const char* odbtcl_tcl_inits[];

void initOdb(Tcl_Interp* interp)
{
  // Define swig TCL commands.
  Odbtcl_Init(interp);
  // Eval encoded sta TCL sources.
  utl::evalTclInit(interp, odb::odbtcl_tcl_inits);
}

}  // namespace odb
