// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "tap/MakeTapcell.h"

#include "ord/OpenRoad.hh"
#include "tap/tapcell.h"
#include "utl/decode.h"

namespace tap {
// Tcl files encoded into strings.
extern const char* tap_tcl_inits[];
}  // namespace tap

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
  utl::evalTclInit(tcl_interp, tap::tap_tcl_inits);
  openroad->getTapcell()->init(openroad->getDb(), openroad->getLogger());
}

}  // namespace ord
