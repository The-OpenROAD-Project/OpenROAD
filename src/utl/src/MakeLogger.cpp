// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "utl/MakeLogger.h"

#include "tcl.h"
#include "utl/Logger.h"
#include "utl/decode.h"

extern "C" {
extern int Utl_Init(Tcl_Interp* interp);
}

namespace utl {
extern const char* utl_tcl_inits[];

void initLogger(Tcl_Interp* tcl_interp)
{
  // Define swig TCL commands.
  Utl_Init(tcl_interp);
  utl::evalTclInit(tcl_interp, utl::utl_tcl_inits);
}

}  // namespace utl
