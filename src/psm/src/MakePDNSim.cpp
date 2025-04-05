// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "psm/MakePDNSim.hh"

#include <tcl.h>

#include "ord/OpenRoad.hh"
#include "psm/pdnsim.h"
#include "sta/StaMain.hh"

namespace sta {
extern const char* psm_tcl_inits[];
}

extern "C" {
extern int Psm_Init(Tcl_Interp* interp);
}

namespace ord {

psm::PDNSim* makePDNSim()
{
  return new psm::PDNSim();
}

void initPDNSim(OpenRoad* openroad)
{
  Tcl_Interp* tcl_interp = openroad->tclInterp();
  Psm_Init(tcl_interp);
  sta::evalTclInit(tcl_interp, sta::psm_tcl_inits);
  openroad->getPDNSim()->init(openroad->getLogger(),
                              openroad->getDb(),
                              openroad->getSta(),
                              openroad->getResizer(),
                              openroad->getOpendp());
}

void deletePDNSim(psm::PDNSim* pdnsim)
{
  delete pdnsim;
}

}  // namespace ord
