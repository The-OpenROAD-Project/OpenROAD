// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "drt/MakeTritonRoute.h"

#include <memory>
#include <utility>

#include "GraphicsFactory.h"
#include "dr/FlexDR_graphics.h"
#include "drt/TritonRoute.h"
#include "pa/FlexPA_graphics.h"
#include "ta/FlexTA_graphics.h"
#include "tcl.h"
#include "utl/decode.h"

extern "C" {
extern int Drt_Init(Tcl_Interp* interp);
}

namespace drt {

// Tcl files encoded into strings.
extern const char* drt_tcl_inits[];

void initGui(drt::TritonRoute* router)
{
  auto graphics_factory = std::make_unique<drt::GraphicsFactory>();
  router->initGraphics(std::move(graphics_factory));
}

void initTcl(Tcl_Interp* tcl_interp)
{
  // Define swig TCL commands.
  Drt_Init(tcl_interp);
  utl::evalTclInit(tcl_interp, drt::drt_tcl_inits);
}

}  // namespace drt
