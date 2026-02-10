// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "db_sta/MakeDbSta.hh"

#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "sta/Sta.hh"
#include "tcl.h"
#include "utl/decode.h"

extern "C" {
extern int Dbsta_Init(Tcl_Interp* interp);
}

namespace sta {

extern const char* dbSta_tcl_inits[];

void deleteDbSta(sta::dbSta* sta)
{
  delete sta;
  sta::Sta::setSta(nullptr);
}

void initDbSta(Tcl_Interp* tcl_interp)
{
  // Define swig TCL commands.
  Dbsta_Init(tcl_interp);
  // Eval encoded sta TCL sources.
  utl::evalTclInit(tcl_interp, sta::dbSta_tcl_inits);
}

}  // namespace sta
