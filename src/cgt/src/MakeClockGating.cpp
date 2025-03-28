// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#include "cgt/MakeClockGating.h"

#include "cgt/ClockGating.h"
#include "ord/OpenRoad.hh"
#include "utl/decode.h"

extern "C" {
extern int Cgt_Init(Tcl_Interp* interp);
}

namespace cgt {

extern const char* cgt_tcl_inits[];

ClockGating* makeClockGating()
{
  return new ClockGating();
}

void initClockGating(ord::OpenRoad* openroad)
{
  Tcl_Interp* tcl_interp = openroad->tclInterp();
  // Define swig TCL commands.
  Cgt_Init(tcl_interp);
  utl::evalTclInit(tcl_interp, cgt::cgt_tcl_inits);
  openroad->getClockGating()->init(openroad->getLogger(), openroad->getSta());
}

void deleteClockGating(cgt::ClockGating* clock_gating)
{
  delete clock_gating;
}

}  // namespace cgt
