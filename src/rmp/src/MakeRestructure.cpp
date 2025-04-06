// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "rmp/MakeRestructure.h"

#include "odb/db.h"
#include "ord/OpenRoad.hh"
#include "rmp/Restructure.h"
#include "sta/StaMain.hh"

namespace sta {
extern const char* rmp_tcl_inits[];
}

extern "C" {
extern int Rmp_Init(Tcl_Interp* interp);
}

namespace ord {

rmp::Restructure* makeRestructure()
{
  return new rmp::Restructure();
}

void initRestructure(OpenRoad* openroad)
{
  Tcl_Interp* tcl_interp = openroad->tclInterp();
  // Define swig TCL commands.
  Rmp_Init(tcl_interp);
  sta::evalTclInit(tcl_interp, sta::rmp_tcl_inits);
  openroad->getRestructure()->init(openroad->getLogger(),
                                   openroad->getSta(),
                                   openroad->getDb(),
                                   openroad->getResizer());
}

void deleteRestructure(rmp::Restructure* restructure)
{
  delete restructure;
}

}  // namespace ord
