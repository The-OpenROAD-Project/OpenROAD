// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "cts/MakeTritoncts.h"

#include "tcl.h"
#include "utl/decode.h"

extern "C" {
extern int Cts_Init(Tcl_Interp* interp);
}

namespace cts {

extern const char* cts_tcl_inits[];

void initTritonCts(Tcl_Interp* tcl_interp)
{
  // Define swig TCL commands.
  Cts_Init(tcl_interp);
  utl::evalTclInit(tcl_interp, cts::cts_tcl_inits);
}

}  // namespace cts
