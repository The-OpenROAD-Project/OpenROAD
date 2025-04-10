// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "par/MakePartitionMgr.h"

#include "ord/OpenRoad.hh"
#include "par/PartitionMgr.h"
#include "sta/StaMain.hh"

namespace sta {
// Tcl files encoded into strings.
extern const char* par_tcl_inits[];
}  // namespace sta

extern "C" {
extern int Par_Init(Tcl_Interp* interp);
}

namespace ord {

par::PartitionMgr* makePartitionMgr()
{
  return new par::PartitionMgr();
}

void initPartitionMgr(OpenRoad* openroad)
{
  Tcl_Interp* tcl_interp = openroad->tclInterp();
  Par_Init(tcl_interp);
  sta::evalTclInit(tcl_interp, sta::par_tcl_inits);

  par::PartitionMgr* kernel = openroad->getPartitionMgr();

  kernel->init(openroad->getDb(),
               openroad->getDbNetwork(),
               openroad->getSta(),
               openroad->getLogger());
};

void deletePartitionMgr(par::PartitionMgr* partitionmgr)
{
  delete partitionmgr;
}
}  // namespace ord
