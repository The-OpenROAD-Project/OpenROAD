// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "pad/MakeICeWall.h"

#include <tcl.h>

#include "ord/OpenRoad.hh"
#include "pad/ICeWall.h"
#include "utl/decode.h"

extern "C" {
extern int Pad_Init(Tcl_Interp* interp);
}

namespace pad {
extern const char* pad_tcl_inits[];
}  // namespace pad

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
  Pad_Init(interp);
  // Eval encoded sta TCL sources.
  utl::evalTclInit(interp, pad::pad_tcl_inits);

  auto* icewall = openroad->getICeWall();
  icewall->init(openroad->getDb(), openroad->getLogger());
}

}  // namespace ord
