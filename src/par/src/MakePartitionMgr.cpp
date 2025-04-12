// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "par/MakePartitionMgr.h"

#include "ord/OpenRoad.hh"
#include "par/PartitionMgr.h"
#include "utl/decode.h"

namespace par {
// Tcl files encoded into strings.
extern const char* par_tcl_inits[];
}  // namespace par

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
  utl::evalTclInit(tcl_interp, par::par_tcl_inits);

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
