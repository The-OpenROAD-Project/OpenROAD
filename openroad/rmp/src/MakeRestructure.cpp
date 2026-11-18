// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "rmp/MakeRestructure.h"

#include "odb/db.h"
#include "rmp/Restructure.h"
#include "tcl.h"
#include "utl/decode.h"

extern "C" {
extern int Rmp_Init(Tcl_Interp* interp);
}

namespace rmp {

extern const char* rmp_tcl_inits[];

void initRestructure(Tcl_Interp* tcl_interp)
{
  // Define swig TCL commands.
  Rmp_Init(tcl_interp);
  utl::evalTclInit(tcl_interp, rmp::rmp_tcl_inits);
}

}  // namespace rmp
