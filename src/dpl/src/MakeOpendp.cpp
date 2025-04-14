// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dpl/MakeOpendp.h"

#include <tcl.h>

#include "dpl/Opendp.h"
#include "ord/OpenRoad.hh"
#include "utl/decode.h"

namespace dpl {
// Tcl files encoded into strings.
extern const char* dpl_tcl_inits[];
}  // namespace dpl

extern "C" {
extern int Dpl_Init(Tcl_Interp* interp);
}

namespace ord {

dpl::Opendp* makeOpendp()
{
  return new dpl::Opendp;
}

void deleteOpendp(dpl::Opendp* opendp)
{
  delete opendp;
}

void initOpendp(OpenRoad* openroad)
{
  Tcl_Interp* tcl_interp = openroad->tclInterp();
  // Define swig TCL commands.
  Dpl_Init(tcl_interp);
  // Eval encoded sta TCL sources.
  utl::evalTclInit(tcl_interp, dpl::dpl_tcl_inits);
  openroad->getOpendp()->init(openroad->getDb(), openroad->getLogger());
}

}  // namespace ord
