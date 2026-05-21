// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "src/dpl/include/dpl/MakeOpendp.h"

#include "src/utl/include/utl/decode.h"
#include "tcl.h"

extern "C" {
extern int Dpl_Init(Tcl_Interp* interp);
}

namespace dpl {

// Tcl files encoded into strings.
extern const char* dpl_tcl_inits[];

void initOpendp(Tcl_Interp* tcl_interp)
{
  // Define swig TCL commands.
  Dpl_Init(tcl_interp);
  // Eval encoded sta TCL sources.
  utl::evalTclInit(tcl_interp, dpl::dpl_tcl_inits);
}

}  // namespace dpl
