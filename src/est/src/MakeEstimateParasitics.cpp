// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "est/MakeEstimateParasitics.h"

#include <memory>
#include <utility>

#include "SteinerRenderer.h"
#include "est/EstimateParasitics.h"
#include "gui/gui.h"
#include "odb/db.h"
#include "tcl.h"
#include "utl/decode.h"

extern "C" {
extern int Est_Init(Tcl_Interp* interp);
}

namespace est {
extern const char* est_tcl_inits[];

void initGui(est::EstimateParasitics* estimate_parasitics)
{
  if (gui::Gui::enabled()) {
    auto steiner_renderer = std::make_unique<SteinerRenderer>();
    estimate_parasitics->initSteinerRenderer(std::move(steiner_renderer));
  }
}

void initTcl(Tcl_Interp* tcl_interp)
{
  // Define swig TCL commands.
  Est_Init(tcl_interp);
  // Eval encoded sta TCL sources.
  utl::evalTclInit(tcl_interp, est::est_tcl_inits);
}

}  // namespace est
