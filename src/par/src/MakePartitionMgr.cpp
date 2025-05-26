// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "par/MakePartitionMgr.h"

#include "par/PartitionMgr.h"
#include "utl/decode.h"

extern "C" {
extern int Par_Init(Tcl_Interp* interp);
}

namespace par {

// Tcl files encoded into strings.
extern const char* par_tcl_inits[];

par::PartitionMgr* makePartitionMgr()
{
  return new par::PartitionMgr();
}

void initPartitionMgr(par::PartitionMgr* partitioner,
                      odb::dbDatabase* db,
                      sta::dbNetwork* db_network,
                      sta::dbSta* sta,
                      utl::Logger* logger,
                      Tcl_Interp* tcl_interp)
{
  Par_Init(tcl_interp);
  utl::evalTclInit(tcl_interp, par::par_tcl_inits);

  partitioner->init(db, db_network, sta, logger);
};

void deletePartitionMgr(par::PartitionMgr* partitionmgr)
{
  delete partitionmgr;
}
}  // namespace par
