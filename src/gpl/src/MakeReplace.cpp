// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#include "src/gpl/include/gpl/MakeReplace.h"

#include "graphicsNone.h"
#include "src/gpl/include/gpl/Replace.h"
#include "src/gpl/src/graphicsImpl.h"
#include "src/gui/include/gui/gui.h"
#include "src/utl/include/utl/Logger.h"
#include "src/utl/include/utl/decode.h"
#include "tcl.h"

extern "C" {
extern int Gpl_Init(Tcl_Interp* interp);
}

namespace gpl {

extern const char* gpl_tcl_inits[];

void initReplace(Tcl_Interp* tcl_interp)
{
  Gpl_Init(tcl_interp);
  utl::evalTclInit(tcl_interp, gpl::gpl_tcl_inits);
}

void initReplaceGraphics(Replace* replace, utl::Logger* log)
{
  if (gui::Gui::get() == nullptr) {
    replace->setGraphicsInterface(gpl::GraphicsNone());
  } else {
    replace->setGraphicsInterface(gpl::GraphicsImpl(log));
  }
}

}  // namespace gpl
