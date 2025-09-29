// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#include "cgt/MakeClockGating.h"

#include "utl/decode.h"

extern "C" {
extern int Cgt_Init(Tcl_Interp* interp);
}

namespace cgt {

extern const char* cgt_tcl_inits[];

void initClockGating(Tcl_Interp* const tcl_interp)
{
  Cgt_Init(tcl_interp);
  utl::evalTclInit(tcl_interp, cgt::cgt_tcl_inits);
}

}  // namespace cgt
