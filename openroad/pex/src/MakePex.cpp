// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "pex/MakePex.h"

#include "tcl.h"
#include "utl/decode.h"

extern "C" {
extern int Pex_Init(Tcl_Interp* interp);
}

namespace pex {

extern const char* pex_tcl_inits[];

void initPex(Tcl_Interp* tcl_interp)
{
  Pex_Init(tcl_interp);
  utl::evalTclInit(tcl_interp, pex::pex_tcl_inits);
}

}  // namespace pex
