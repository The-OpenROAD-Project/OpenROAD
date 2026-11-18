// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "ifp/MakeInitFloorplan.hh"

#include "tcl.h"
#include "utl/decode.h"

extern "C" {
extern int Ifp_Init(Tcl_Interp* interp);
}

namespace ifp {

extern const char* ifp_tcl_inits[];

void initInitFloorplan(Tcl_Interp* tcl_interp)
{
  // Define swig TCL commands.
  Ifp_Init(tcl_interp);
  // Eval encoded sta TCL sources.
  utl::evalTclInit(tcl_interp, ifp::ifp_tcl_inits);
}

}  // namespace ifp
