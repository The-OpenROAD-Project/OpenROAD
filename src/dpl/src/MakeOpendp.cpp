// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dpl/MakeOpendp.h"

#include <tcl.h>

#include "dpl/Opendp.h"
#include "utl/decode.h"

extern "C" {
extern int Dpl_Init(Tcl_Interp* interp);
}

namespace dpl {

// Tcl files encoded into strings.
extern const char* dpl_tcl_inits[];

dpl::Opendp* makeOpendp()
{
  return new dpl::Opendp;
}

void deleteOpendp(dpl::Opendp* opendp)
{
  delete opendp;
}

void initOpendp(dpl::Opendp* dpl,
                odb::dbDatabase* db,
                utl::Logger* logger,
                Tcl_Interp* tcl_interp)
{
  // Define swig TCL commands.
  Dpl_Init(tcl_interp);
  // Eval encoded sta TCL sources.
  utl::evalTclInit(tcl_interp, dpl::dpl_tcl_inits);
  dpl->init(db, logger);
}

}  // namespace dpl
