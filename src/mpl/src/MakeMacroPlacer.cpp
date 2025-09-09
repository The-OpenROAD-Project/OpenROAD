// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "mpl/MakeMacroPlacer.h"

#include <tcl.h>

#include "mpl/rtl_mp.h"
#include "utl/decode.h"

extern "C" {
extern int Mpl_Init(Tcl_Interp* interp);
}

namespace mpl {

extern const char* mpl_tcl_inits[];

mpl::MacroPlacer* makeMacroPlacer()
{
  return new mpl::MacroPlacer;
}

void initMacroPlacer(mpl::MacroPlacer* macro_placer,
                     sta::dbNetwork* network,
                     odb::dbDatabase* db,
                     sta::dbSta* sta,
                     utl::Logger* logger,
                     par::PartitionMgr* tritonpart,
                     Tcl_Interp* tcl_interp)
{
  Mpl_Init(tcl_interp);
  utl::evalTclInit(tcl_interp, mpl::mpl_tcl_inits);
  macro_placer->init(network, db, sta, logger, tritonpart);
}

void deleteMacroPlacer(mpl::MacroPlacer* macro_placer)
{
  delete macro_placer;
}

}  // namespace mpl
