// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "fsv/MakeFieldSolver.h"

#include "tcl.h"
#include "utl/decode.h"

extern "C" {
extern int Fsv_Init(Tcl_Interp* interp);
}

namespace fsv {
// Tcl files encoded into strings.
extern const char* fsv_tcl_inits[];

void initFieldSolver(Tcl_Interp* tcl_interp)
{
  Fsv_Init(tcl_interp);
  utl::evalTclInit(tcl_interp, fsv::fsv_tcl_inits);
}

}  // namespace fsv
