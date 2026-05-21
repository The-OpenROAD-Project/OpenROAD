// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "src/psm/include/psm/MakePDNSim.hh"

#include "src/psm/include/psm/pdnsim.h"
#include "src/utl/include/utl/decode.h"
#include "tcl.h"

extern "C" {
extern int Psm_Init(Tcl_Interp* interp);
}

namespace psm {

extern const char* psm_tcl_inits[];

void initPDNSim(Tcl_Interp* tcl_interp)
{
  Psm_Init(tcl_interp);
  utl::evalTclInit(tcl_interp, psm::psm_tcl_inits);
}

}  // namespace psm
