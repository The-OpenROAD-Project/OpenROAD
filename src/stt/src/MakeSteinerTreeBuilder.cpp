// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "stt/MakeSteinerTreeBuilder.h"

#include "stt/SteinerTreeBuilder.h"
#include "tcl.h"
#include "utl/decode.h"

extern "C" {
extern int Stt_Init(Tcl_Interp* interp);
}

namespace stt {
// Tcl files encoded into strings.
extern const char* stt_tcl_inits[];

void initSteinerTreeBuilder(Tcl_Interp* tcl_interp)
{
  // Define swig TCL commands.
  Stt_Init(tcl_interp);
  utl::evalTclInit(tcl_interp, stt::stt_tcl_inits);
}

}  // namespace stt
