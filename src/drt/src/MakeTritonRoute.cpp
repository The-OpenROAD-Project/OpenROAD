// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "src/drt/include/drt/MakeTritonRoute.h"

#include <memory>
#include <utility>

#include "src/drt/include/drt/TritonRoute.h"
#include "src/drt/src/GraphicsFactory.h"
#include "src/drt/src/dr/FlexDR_graphics.h"
#include "src/drt/src/pa/FlexPA_graphics.h"
#include "src/drt/src/ta/FlexTA_graphics.h"
#include "src/utl/include/utl/decode.h"
#include "tcl.h"

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
