// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#include "gpl/MakeReplace.h"

#include <tcl.h>

#include "gpl/Replace.h"
#include "ord/OpenRoad.hh"
#include "utl/decode.h"

namespace gpl {
extern const char* gpl_tcl_inits[];
}

extern "C" {
extern int Gpl_Init(Tcl_Interp* interp);
}

namespace ord {

gpl::Replace* makeReplace()
{
  return new gpl::Replace();
}

void initReplace(OpenRoad* openroad)
{
  Tcl_Interp* tcl_interp = openroad->tclInterp();
  Gpl_Init(tcl_interp);
  utl::evalTclInit(tcl_interp, gpl::gpl_tcl_inits);
  openroad->getReplace()->init(openroad->getDb(),
                               openroad->getSta(),
                               openroad->getResizer(),
                               openroad->getGlobalRouter(),
                               openroad->getLogger());
}

void deleteReplace(gpl::Replace* replace)
{
  delete replace;
}

}  // namespace ord
