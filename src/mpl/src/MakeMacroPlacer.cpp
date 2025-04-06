// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "mpl/MakeMacroPlacer.h"

#include <tcl.h>

#include "mpl/rtl_mp.h"
#include "ord/OpenRoad.hh"
#include "sta/StaMain.hh"

namespace sta {
extern const char* mpl_tcl_inits[];
}

extern "C" {
extern int Mpl_Init(Tcl_Interp* interp);
}

namespace ord {

mpl::MacroPlacer* makeMacroPlacer()
{
  return new mpl::MacroPlacer;
}

void initMacroPlacer(OpenRoad* openroad)
{
  Tcl_Interp* tcl_interp = openroad->tclInterp();
  Mpl_Init(tcl_interp);
  sta::evalTclInit(tcl_interp, sta::mpl_tcl_inits);
  openroad->getMacroPlacer()->init(openroad->getDbNetwork(),
                                   openroad->getDb(),
                                   openroad->getSta(),
                                   openroad->getLogger(),
                                   openroad->getPartitionMgr());
}

void deleteMacroPlacer(mpl::MacroPlacer* macro_placer)
{
  delete macro_placer;
}

}  // namespace ord
