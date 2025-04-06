// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "dpo/MakeOptdp.h"

#include <tcl.h>

#include "dpo/Optdp.h"
#include "ord/OpenRoad.hh"
#include "sta/StaMain.hh"

namespace sta {
// Tcl files encoded into strings.
extern const char* dpo_tcl_inits[];
}  // namespace sta

extern "C" {
extern int Dpo_Init(Tcl_Interp* interp);
}

namespace ord {

dpo::Optdp* makeOptdp()
{
  return new dpo::Optdp;
}

void deleteOptdp(dpo::Optdp* optdp)
{
  delete optdp;
}

void initOptdp(OpenRoad* openroad)
{
  Tcl_Interp* tcl_interp = openroad->tclInterp();
  // Define swig TCL commands.
  Dpo_Init(tcl_interp);
  // Eval encoded sta TCL sources.
  sta::evalTclInit(tcl_interp, sta::dpo_tcl_inits);
  // openroad->getOptdp()->init(openroad);
  openroad->getOptdp()->init(
      openroad->getDb(), openroad->getLogger(), openroad->getOpendp());
}

}  // namespace ord
