// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "src/par/include/par/MakePartitionMgr.h"

#include "src/utl/include/utl/decode.h"
#include "tcl.h"

extern "C" {
extern int Par_Init(Tcl_Interp* interp);
}

namespace par {

// Tcl files encoded into strings.
extern const char* par_tcl_inits[];

void initPartitionMgr(Tcl_Interp* tcl_interp)
{
  Par_Init(tcl_interp);
  utl::evalTclInit(tcl_interp, par::par_tcl_inits);
};

}  // namespace par
