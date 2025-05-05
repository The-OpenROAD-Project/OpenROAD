// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "triton_route/MakeTritonRoute.h"

#include <memory>
#include <utility>

#include "GraphicsFactory.h"
#include "dr/FlexDR_graphics.h"
#include "pa/FlexPA_graphics.h"
#include "ta/FlexTA_graphics.h"
#include "triton_route/TritonRoute.h"
#include "utl/decode.h"

extern "C" {
extern int Drt_Init(Tcl_Interp* interp);
}

namespace drt {

// Tcl files encoded into strings.
extern const char* drt_tcl_inits[];

drt::TritonRoute* makeTritonRoute()
{
  return new drt::TritonRoute();
}

void deleteTritonRoute(drt::TritonRoute* router)
{
  delete router;
}

void initTritonRoute(drt::TritonRoute* router,
                     odb::dbDatabase* db,
                     utl::Logger* logger,
                     dst::Distributed* dist,
                     stt::SteinerTreeBuilder* stt_builder,
                     Tcl_Interp* tcl_interp)
{
  // Define swig TCL commands.
  Drt_Init(tcl_interp);
  utl::evalTclInit(tcl_interp, drt::drt_tcl_inits);

  std::unique_ptr<drt::AbstractGraphicsFactory> graphics_factory
      = std::make_unique<drt::GraphicsFactory>();

  router->init(db, logger, dist, stt_builder, std::move(graphics_factory));
}

}  // namespace drt
