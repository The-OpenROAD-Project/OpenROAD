// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "dst/MakeDistributed.h"

#include <tcl.h>

#include "dst/Distributed.h"
#include "ord/OpenRoad.hh"
#include "utl/decode.h"

namespace dst {
// Tcl files encoded into strings.
extern const char* dst_tcl_inits[];
}  // namespace dst

extern "C" {
extern int Dst_Init(Tcl_Interp* interp);
}

namespace ord {

dst::Distributed* makeDistributed()
{
  return new dst::Distributed();
}

void deleteDistributed(dst::Distributed* dstr)
{
  delete dstr;
}

void initDistributed(OpenRoad* openroad)
{
  // Define swig TCL commands.
  auto tcl_interp = openroad->tclInterp();
  Dst_Init(tcl_interp);
  utl::evalTclInit(tcl_interp, dst::dst_tcl_inits);
  openroad->getDistributed()->init(openroad->getLogger());
}

}  // namespace ord
