// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#include "gpl/MakeReplace.h"

#include <tcl.h>

#include "gpl/Replace.h"
#include "utl/decode.h"

extern "C" {
extern int Gpl_Init(Tcl_Interp* interp);
}

namespace gpl {

extern const char* gpl_tcl_inits[];

gpl::Replace* makeReplace()
{
  return new gpl::Replace();
}

void initReplace(gpl::Replace* replace,
                 odb::dbDatabase* db,
                 sta::dbSta* sta,
                 rsz::Resizer* resizer,
                 grt::GlobalRouter* global_route,
                 utl::Logger* logger,
                 Tcl_Interp* tcl_interp)
{
  Gpl_Init(tcl_interp);
  utl::evalTclInit(tcl_interp, gpl::gpl_tcl_inits);
  replace->init(db, sta, resizer, global_route, logger);
}

void deleteReplace(gpl::Replace* replace)
{
  delete replace;
}

}  // namespace gpl
