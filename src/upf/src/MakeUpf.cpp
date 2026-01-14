// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "upf/MakeUpf.h"

#include "tcl.h"
#include "utl/decode.h"

extern "C" {
extern int Upf_Init(Tcl_Interp* interp);
}

namespace upf {
extern const char* upf_tcl_inits[];

void initUpf(Tcl_Interp* tcl_interp)
{
  // Define swig TCL commands.
  Upf_Init(tcl_interp);
  // Eval encoded sta TCL sources.
  utl::evalTclInit(tcl_interp, upf::upf_tcl_inits);
}

}  // namespace upf
