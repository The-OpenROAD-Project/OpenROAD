// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "tap/MakeTapcell.h"

#include "ord/OpenRoad.hh"
#include "sta/StaMain.hh"
#include "tap/tapcell.h"

namespace sta {
// Tcl files encoded into strings.
extern const char* tap_tcl_inits[];
}  // namespace sta

extern "C" {
extern int Tap_Init(Tcl_Interp* interp);
}

namespace ord {

tap::Tapcell* makeTapcell()
{
  return new tap::Tapcell();
}

void deleteTapcell(tap::Tapcell* tapcell)
{
  delete tapcell;
}

void initTapcell(OpenRoad* openroad)
{
  Tcl_Interp* tcl_interp = openroad->tclInterp();
  Tap_Init(tcl_interp);
  // Eval encoded sta TCL sources.
  sta::evalTclInit(tcl_interp, sta::tap_tcl_inits);
  openroad->getTapcell()->init(openroad->getDb(), openroad->getLogger());
}

}  // namespace ord
