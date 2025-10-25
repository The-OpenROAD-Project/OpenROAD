// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "fin/MakeFinale.h"

#include "tcl.h"
#include "utl/decode.h"

extern "C" {
extern int Fin_Init(Tcl_Interp* interp);
}

namespace fin {

// Tcl files encoded into strings.
extern const char* fin_tcl_inits[];

void initFinale(Tcl_Interp* tcl_interp)
{
  // Define swig TCL commands.
  Fin_Init(tcl_interp);
  // Eval encoded sta TCL sources.
  utl::evalTclInit(tcl_interp, fin::fin_tcl_inits);
}

}  // namespace fin
