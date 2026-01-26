// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "psm/MakePDNSim.hh"

#include "psm/pdnsim.h"
#include "tcl.h"
#include "utl/decode.h"

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
