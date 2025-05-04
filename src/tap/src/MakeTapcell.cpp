// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "tap/MakeTapcell.h"

#include "tap/tapcell.h"
#include "utl/decode.h"

extern "C" {
extern int Tap_Init(Tcl_Interp* interp);
}

namespace tap {
// Tcl files encoded into strings.
extern const char* tap_tcl_inits[];

tap::Tapcell* makeTapcell()
{
  return new tap::Tapcell();
}

void deleteTapcell(tap::Tapcell* tapcell)
{
  delete tapcell;
}

void initTapcell(tap::Tapcell* tapcell,
                 odb::dbDatabase* db,
                 utl::Logger* logger,
                 Tcl_Interp* tcl_interp)
{
  Tap_Init(tcl_interp);
  // Eval encoded sta TCL sources.
  utl::evalTclInit(tcl_interp, tap::tap_tcl_inits);
  tapcell->init(db, logger);
}

}  // namespace tap
