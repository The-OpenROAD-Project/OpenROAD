// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "rmp/MakeRestructure.h"

#include "odb/db.h"
#include "rmp/Restructure.h"
#include "utl/decode.h"

extern "C" {
extern int Rmp_Init(Tcl_Interp* interp);
}

namespace rmp {

extern const char* rmp_tcl_inits[];

rmp::Restructure* makeRestructure()
{
  return new rmp::Restructure();
}

void initRestructure(rmp::Restructure* restructure,
                     utl::Logger* logger,
                     sta::dbSta* sta,
                     odb::dbDatabase* db,
                     rsz::Resizer* resizer,
                     Tcl_Interp* tcl_interp)
{
  // Define swig TCL commands.
  Rmp_Init(tcl_interp);
  utl::evalTclInit(tcl_interp, rmp::rmp_tcl_inits);
  restructure->init(logger, sta, db, resizer);
}

void deleteRestructure(rmp::Restructure* restructure)
{
  delete restructure;
}

}  // namespace rmp
