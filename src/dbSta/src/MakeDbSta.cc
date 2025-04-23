// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "db_sta/MakeDbSta.hh"

#include <tcl.h>

#include <memory>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "ord/OpenRoad.hh"
#include "utl/decode.h"

extern "C" {
extern int Dbsta_Init(Tcl_Interp* interp);
}

namespace sta {
extern const char* dbSta_tcl_inits[];
}

namespace ord {

using sta::dbSta;

void deleteDbSta(sta::dbSta* sta)
{
  delete sta;
  sta::Sta::setSta(nullptr);
}

void initDbSta(OpenRoad* openroad)
{
  dbSta* sta = openroad->getSta();
  sta::initSta();

  utl::Logger* logger = openroad->getLogger();
  sta->initVars(openroad->tclInterp(), openroad->getDb(), logger);
  sta::Sta::setSta(sta);

  Tcl_Interp* tcl_interp = openroad->tclInterp();

  // Define swig TCL commands.
  Dbsta_Init(tcl_interp);
  // Eval encoded sta TCL sources.
  utl::evalTclInit(tcl_interp, sta::dbSta_tcl_inits);
}

}  // namespace ord
