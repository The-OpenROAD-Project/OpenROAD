// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "triton_route/MakeTritonRoute.h"

#include "GraphicsFactory.h"
#include "dr/FlexDR_graphics.h"
#include "ord/OpenRoad.hh"
#include "pa/FlexPA_graphics.h"
#include "sta/StaMain.hh"
#include "ta/FlexTA_graphics.h"
#include "triton_route/TritonRoute.h"

namespace sta {
// Tcl files encoded into strings.
extern const char* drt_tcl_inits[];
}  // namespace sta

extern "C" {
extern int Drt_Init(Tcl_Interp* interp);
}

namespace ord {

drt::TritonRoute* makeTritonRoute()
{
  return new drt::TritonRoute();
}

void deleteTritonRoute(drt::TritonRoute* router)
{
  delete router;
}

void initTritonRoute(OpenRoad* openroad)
{
  // Define swig TCL commands.
  auto tcl_interp = openroad->tclInterp();
  Drt_Init(tcl_interp);
  sta::evalTclInit(tcl_interp, sta::drt_tcl_inits);

  drt::TritonRoute* router = openroad->getTritonRoute();
  std::unique_ptr<drt::AbstractGraphicsFactory> graphics_factory
      = std::make_unique<drt::GraphicsFactory>();

  router->init(openroad->getDb(),
               openroad->getLogger(),
               openroad->getDistributed(),
               openroad->getSteinerTreeBuilder(),
               std::move(graphics_factory));
}

}  // namespace ord
