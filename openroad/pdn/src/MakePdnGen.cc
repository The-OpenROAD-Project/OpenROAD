// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "pdn/MakePdnGen.hh"

#include "tcl.h"
#include "utl/decode.h"

extern "C" {
extern int Pdn_Init(Tcl_Interp* interp);
}

namespace pdn {

extern const char* pdn_tcl_inits[];

void initPdnGen(Tcl_Interp* tcl_interp)
{
  // Define swig TCL commands.
  Pdn_Init(tcl_interp);
  // Eval encoded sta TCL sources.
  utl::evalTclInit(tcl_interp, pdn::pdn_tcl_inits);
}

}  // namespace pdn
