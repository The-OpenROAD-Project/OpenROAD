// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "upf/MakeUpf.h"

#include <tcl.h>

#include "ord/OpenRoad.hh"
#include "utl/decode.h"

namespace sta {

extern "C" {
extern int Upf_Init(Tcl_Interp* interp);
}
}  // namespace sta

namespace upf {
extern const char* upf_tcl_inits[];
}

namespace ord {

void initUpf(OpenRoad* openroad)
{
  Tcl_Interp* interp = openroad->tclInterp();
  // Define swig TCL commands.
  sta::Upf_Init(interp);
  // Eval encoded sta TCL sources.
  utl::evalTclInit(interp, upf::upf_tcl_inits);
}

}  // namespace ord
