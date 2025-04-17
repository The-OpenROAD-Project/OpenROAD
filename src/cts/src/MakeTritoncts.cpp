// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "cts/MakeTritoncts.h"

#include "CtsOptions.h"
#include "cts/TritonCTS.h"
#include "odb/db.h"
#include "ord/OpenRoad.hh"
#include "utl/decode.h"

namespace cts {
// Tcl files encoded into strings.
extern const char* cts_tcl_inits[];
}  // namespace cts

extern "C" {
extern int Cts_Init(Tcl_Interp* interp);
}

namespace ord {

cts::TritonCTS* makeTritonCts()
{
  return new cts::TritonCTS();
}

void initTritonCts(OpenRoad* openroad)
{
  Tcl_Interp* tcl_interp = openroad->tclInterp();
  // Define swig TCL commands.
  Cts_Init(tcl_interp);
  utl::evalTclInit(tcl_interp, cts::cts_tcl_inits);
  openroad->getTritonCts()->init(openroad->getLogger(),
                                 openroad->getDb(),
                                 openroad->getDbNetwork(),
                                 openroad->getSta(),
                                 openroad->getSteinerTreeBuilder(),
                                 openroad->getResizer());
}

void deleteTritonCts(cts::TritonCTS* tritoncts)
{
  delete tritoncts;
}

}  // namespace ord
