// Copyright 2023 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "db_sta/MakeDbSta.hh"

#include <tcl.h>

#include "PathRenderer.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "gui/gui.h"
#include "heatMap.h"
#include "odb/db.h"
#include "ord/OpenRoad.hh"
#include "sta/StaMain.hh"

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

  if (gui::Gui::enabled()) {
    sta->setPathRenderer(std::make_unique<sta::PathRenderer>(sta));
  }
  sta->setPowerDensityDataSource(
      std::make_unique<sta::PowerDensityDataSource>(sta, logger));

  Tcl_Interp* tcl_interp = openroad->tclInterp();

  // Define swig TCL commands.
  Dbsta_Init(tcl_interp);
  // Eval encoded sta TCL sources.
  sta::evalTclInit(tcl_interp, sta::dbSta_tcl_inits);

  openroad->addObserver(sta);
}

}  // namespace ord
