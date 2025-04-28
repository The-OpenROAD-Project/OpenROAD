// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "fin/MakeFinale.h"

#include <tcl.h>

#include "fin/Finale.h"
#include "ord/OpenRoad.hh"
#include "utl/decode.h"

namespace fin {
// Tcl files encoded into strings.
extern const char* fin_tcl_inits[];
}  // namespace fin

extern "C" {
extern int Fin_Init(Tcl_Interp* interp);
}

namespace ord {

fin::Finale* makeFinale()
{
  return new fin::Finale;
}

void deleteFinale(fin::Finale* finale)
{
  delete finale;
}

void initFinale(OpenRoad* openroad)
{
  Tcl_Interp* tcl_interp = openroad->tclInterp();
  // Define swig TCL commands.
  Fin_Init(tcl_interp);
  // Eval encoded sta TCL sources.
  utl::evalTclInit(tcl_interp, fin::fin_tcl_inits);
  openroad->getFinale()->init(openroad->getDb(), openroad->getLogger());
}

}  // namespace ord
