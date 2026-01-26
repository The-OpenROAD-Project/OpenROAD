// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "rcx/MakeOpenRCX.h"

#include "rcx/ext.h"
#include "tcl.h"
#include "utl/decode.h"

extern "C" {
extern int Rcx_Init(Tcl_Interp* interp);
}

namespace rcx {

// Tcl files encoded into strings.
extern const char* rcx_tcl_inits[];

void initOpenRCX(Tcl_Interp* tcl_interp)
{
  Rcx_Init(tcl_interp);
  utl::evalTclInit(tcl_interp, rcx::rcx_tcl_inits);
}

}  // namespace rcx
