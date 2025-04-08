// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "stt/MakeSteinerTreeBuilder.h"

#include "ord/OpenRoad.hh"
#include "sta/StaMain.hh"
#include "stt/SteinerTreeBuilder.h"

namespace sta {
// Tcl files encoded into strings.
extern const char* stt_tcl_inits[];
}  // namespace sta

extern "C" {
extern int Stt_Init(Tcl_Interp* interp);
}

namespace ord {

stt::SteinerTreeBuilder* makeSteinerTreeBuilder()
{
  return new stt::SteinerTreeBuilder();
}

void deleteSteinerTreeBuilder(stt::SteinerTreeBuilder* stt_builder)
{
  delete stt_builder;
}

void initSteinerTreeBuilder(OpenRoad* openroad)
{
  Tcl_Interp* tcl_interp = openroad->tclInterp();
  // Define swig TCL commands.
  Stt_Init(tcl_interp);
  sta::evalTclInit(tcl_interp, sta::stt_tcl_inits);
  openroad->getSteinerTreeBuilder()->init(openroad->getDb(),
                                          openroad->getLogger());
}

}  // namespace ord
