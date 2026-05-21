// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "src/dst/include/dst/MakeDistributed.h"

#include "src/utl/include/utl/decode.h"
#include "tcl.h"

extern "C" {
extern int Dst_Init(Tcl_Interp* interp);
}

namespace dst {

// Tcl files encoded into strings.
extern const char* dst_tcl_inits[];

void initDistributed(Tcl_Interp* tcl_interp)
{
  // Define swig TCL commands.
  Dst_Init(tcl_interp);
  utl::evalTclInit(tcl_interp, dst::dst_tcl_inits);
}

}  // namespace dst
