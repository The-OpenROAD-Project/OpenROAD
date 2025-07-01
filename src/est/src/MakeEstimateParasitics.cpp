// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "est/MakeEstimateParasitics.h"

#include <memory>
#include <utility>

#include "gui/gui.h"
#include "est/EstimateParasitics.h"
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
                 odb::dbDatabase* db,
                 sta::dbSta* sta,
                 stt::SteinerTreeBuilder* stt_builder,
                 grt::GlobalRouter* global_router)
{
  estimate_parasitics->init(logger,
                db,
                sta,
                stt_builder,
                global_router);
  // Define swig TCL commands.
  Est_Init(tcl_interp);
  // Eval encoded sta TCL sources.
  utl::evalTclInit(tcl_interp, est::est_tcl_inits);
}

}  // namespace est
