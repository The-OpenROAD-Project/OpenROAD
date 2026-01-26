// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "ant/MakeAntennaChecker.hh"

#include "tcl.h"
#include "utl/decode.h"

extern "C" {
extern int Ant_Init(Tcl_Interp* interp);
}

namespace ant {

// Tcl files encoded into strings.
extern const char* ant_tcl_inits[];

void initAntennaChecker(Tcl_Interp* tcl_interp)
{
  Ant_Init(tcl_interp);
  utl::evalTclInit(tcl_interp, ant::ant_tcl_inits);
}

}  // namespace ant
