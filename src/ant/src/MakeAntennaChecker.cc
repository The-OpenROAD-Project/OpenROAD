// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "ant/MakeAntennaChecker.hh"

#include "ant/AntennaChecker.hh"
#include "grt/GlobalRouter.h"
#include "ord/OpenRoad.hh"
#include "sta/StaMain.hh"

namespace sta {
// Tcl files encoded into strings.
extern const char* ant_tcl_inits[];
}  // namespace sta

extern "C" {
extern int Ant_Init(Tcl_Interp* interp);
}

namespace ord {

ant::AntennaChecker* makeAntennaChecker()
{
  return new ant::AntennaChecker;
}

void deleteAntennaChecker(ant::AntennaChecker* antenna_checker)
{
  delete antenna_checker;
}

void initAntennaChecker(OpenRoad* openroad)
{
  Tcl_Interp* tcl_interp = openroad->tclInterp();

  Ant_Init(tcl_interp);
  sta::evalTclInit(tcl_interp, sta::ant_tcl_inits);
  openroad->getAntennaChecker()->init(
      openroad->getDb(), openroad->getGlobalRouter(), openroad->getLogger());
}

}  // namespace ord
