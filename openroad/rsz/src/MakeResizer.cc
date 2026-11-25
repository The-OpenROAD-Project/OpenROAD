// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "rsz/MakeResizer.hh"

#include "odb/db.h"
#include "rsz/Resizer.hh"
#include "tcl.h"
#include "utl/decode.h"

extern "C" {
extern int Rsz_Init(Tcl_Interp* interp);
}

namespace rsz {
extern const char* rsz_tcl_inits[];

void initResizer(Tcl_Interp* tcl_interp)
{
  // Define swig TCL commands.
  Rsz_Init(tcl_interp);
  // Eval encoded sta TCL sources.
  utl::evalTclInit(tcl_interp, rsz::rsz_tcl_inits);
}

}  // namespace rsz
