// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "grt/MakeGlobalRouter.h"

#include "FastRoute.h"
#include "grt/GlobalRouter.h"
#include "heatMap.h"
#include "heatMapRudy.h"
#include "ord/OpenRoad.hh"
#include "sta/StaMain.hh"

namespace sta {
// Tcl files encoded into strings.
extern const char* grt_tcl_inits[];
}  // namespace sta

extern "C" {
extern int Grt_Init(Tcl_Interp* interp);
}

namespace ord {

grt::GlobalRouter* makeGlobalRouter()
{
  return new grt::GlobalRouter();
}

void deleteGlobalRouter(grt::GlobalRouter* global_router)
{
  delete global_router;
}

void initGlobalRouter(OpenRoad* openroad)
{
  Tcl_Interp* tcl_interp = openroad->tclInterp();
  // Define swig TCL commands.
  Grt_Init(tcl_interp);
  sta::evalTclInit(tcl_interp, sta::grt_tcl_inits);
  openroad->getGlobalRouter()->init(
      openroad->getLogger(),
      openroad->getSteinerTreeBuilder(),
      openroad->getDb(),
      openroad->getSta(),
      openroad->getResizer(),
      openroad->getAntennaChecker(),
      openroad->getOpendp(),
      std::make_unique<grt::RoutingCongestionDataSource>(openroad->getLogger(),
                                                         openroad->getDb()),
      std::make_unique<grt::RUDYDataSource>(openroad->getLogger(),
                                            openroad->getGlobalRouter(),
                                            openroad->getDb()));
}

}  // namespace ord
