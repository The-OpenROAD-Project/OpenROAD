// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "ppl/MakeIoplacer.h"

#include "ord/OpenRoad.hh"
#include "ppl/IOPlacer.h"
#include "sta/StaMain.hh"

namespace sta {
// Tcl files encoded into strings.
extern const char* ppl_tcl_inits[];
}  // namespace sta

extern "C" {
extern int Ppl_Init(Tcl_Interp* interp);
}

namespace ord {

ppl::IOPlacer* makeIoplacer()
{
  return new ppl::IOPlacer();
}

void deleteIoplacer(ppl::IOPlacer* ioplacer)
{
  delete ioplacer;
}

void initIoplacer(OpenRoad* openroad)
{
  Tcl_Interp* tcl_interp = openroad->tclInterp();
  // Define swig TCL commands.
  Ppl_Init(tcl_interp);
  sta::evalTclInit(tcl_interp, sta::ppl_tcl_inits);

  openroad->getIOPlacer()->init(openroad->getDb(), openroad->getLogger());
}

}  // namespace ord
