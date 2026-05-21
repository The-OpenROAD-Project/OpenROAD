// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "src/mpl/include/mpl/MakeMacroPlacer.h"

#include "src/utl/include/utl/decode.h"
#include "tcl.h"

extern "C" {
extern int Mpl_Init(Tcl_Interp* interp);
}

namespace mpl {

extern const char* mpl_tcl_inits[];

void initMacroPlacer(Tcl_Interp* tcl_interp)
{
  Mpl_Init(tcl_interp);
  utl::evalTclInit(tcl_interp, mpl::mpl_tcl_inits);
}

}  // namespace mpl
