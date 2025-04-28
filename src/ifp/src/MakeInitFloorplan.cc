// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "ifp/MakeInitFloorplan.hh"

#include <tcl.h>

#include "ord/OpenRoad.hh"
#include "utl/decode.h"

namespace sta {

extern "C" {
extern int Ifp_Init(Tcl_Interp* interp);
}
}  // namespace sta

namespace ifp {
extern const char* ifp_tcl_inits[];
}

namespace ord {

void initInitFloorplan(OpenRoad* openroad)
{
  Tcl_Interp* interp = openroad->tclInterp();
  // Define swig TCL commands.
  sta::Ifp_Init(interp);
  // Eval encoded sta TCL sources.
  utl::evalTclInit(interp, ifp::ifp_tcl_inits);
}

}  // namespace ord
