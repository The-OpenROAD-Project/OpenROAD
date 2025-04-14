// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "ant/MakeAntennaChecker.hh"

#include "ant/AntennaChecker.hh"
#include "grt/GlobalRouter.h"
#include "ord/OpenRoad.hh"
#include "utl/decode.h"

namespace ant {
// Tcl files encoded into strings.
extern const char* ant_tcl_inits[];
}  // namespace ant

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
  utl::evalTclInit(tcl_interp, ant::ant_tcl_inits);
  openroad->getAntennaChecker()->init(
      openroad->getDb(), openroad->getGlobalRouter(), openroad->getLogger());
}

}  // namespace ord
