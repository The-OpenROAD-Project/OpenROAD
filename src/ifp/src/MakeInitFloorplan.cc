// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "ifp/MakeInitFloorplan.hh"

#include <tcl.h>

#include "ord/OpenRoad.hh"
#include "sta/StaMain.hh"

namespace sta {

extern "C" {
extern int Ifp_Init(Tcl_Interp* interp);
}

extern const char* ifp_tcl_inits[];

}  // namespace sta

namespace ord {

void initInitFloorplan(OpenRoad* openroad)
{
  Tcl_Interp* interp = openroad->tclInterp();
  // Define swig TCL commands.
  sta::Ifp_Init(interp);
  // Eval encoded sta TCL sources.
  sta::evalTclInit(interp, sta::ifp_tcl_inits);
}

}  // namespace ord
