// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "pad/MakeICeWall.h"

#include <tcl.h>

#include "ord/OpenRoad.hh"
#include "pad/ICeWall.h"
#include "sta/StaMain.hh"

namespace sta {

extern "C" {
extern int Pad_Init(Tcl_Interp* interp);
}

extern const char* pad_tcl_inits[];

}  // namespace sta

namespace ord {

pad::ICeWall* makeICeWall()
{
  return new pad::ICeWall();
}

void deleteICeWall(pad::ICeWall* icewall)
{
  delete icewall;
}

void initICeWall(OpenRoad* openroad)
{
  Tcl_Interp* interp = openroad->tclInterp();
  // Define swig TCL commands.
  sta::Pad_Init(interp);
  // Eval encoded sta TCL sources.
  sta::evalTclInit(interp, sta::pad_tcl_inits);

  auto* icewall = openroad->getICeWall();
  icewall->init(openroad->getDb(), openroad->getLogger());
}

}  // namespace ord
