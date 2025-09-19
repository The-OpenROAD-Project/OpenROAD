// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "est/MakeEstimateParasitics.h"

#include <memory>
#include <utility>

#include "SteinerRenderer.h"
#include "est/EstimateParasitics.h"
#include "gui/gui.h"
#include "odb/db.h"
#include "utl/decode.h"

extern "C" {
extern int Est_Init(Tcl_Interp* interp);
}

namespace est {
extern const char* est_tcl_inits[];

est::EstimateParasitics* makeEstimateParasitics()
{
  return new est::EstimateParasitics;
}

void deleteEstimateParasitics(est::EstimateParasitics* estimate_parasitics)
{
  delete estimate_parasitics;
}

void initEstimateParasitics(est::EstimateParasitics* estimate_parasitics,
                            Tcl_Interp* tcl_interp,
                            utl::Logger* logger,
                            utl::CallBackHandler* callback_handler,
                            odb::dbDatabase* db,
                            sta::dbSta* sta,
                            stt::SteinerTreeBuilder* stt_builder,
                            grt::GlobalRouter* global_router)
{
  estimate_parasitics->init(
      logger, callback_handler, db, sta, stt_builder, global_router);
  std::unique_ptr<est::AbstractSteinerRenderer> steiner_renderer;
  if (gui::Gui::enabled()) {
    steiner_renderer = std::make_unique<SteinerRenderer>();
    estimate_parasitics->initSteinerRenderer(std::move(steiner_renderer));
  }
  // Define swig TCL commands.
  Est_Init(tcl_interp);
  // Eval encoded sta TCL sources.
  utl::evalTclInit(tcl_interp, est::est_tcl_inits);
}

}  // namespace est
