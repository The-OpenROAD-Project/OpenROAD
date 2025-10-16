// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#include "gpl/MakeReplace.h"

#include <tcl.h>

#include "gpl/Replace.h"
#include "graphicsImpl.h"
#include "utl/Logger.h"
#include "utl/decode.h"

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
  replace->setGraphicsInterface(gpl::GraphicsImpl(log));
}

}  // namespace gpl
