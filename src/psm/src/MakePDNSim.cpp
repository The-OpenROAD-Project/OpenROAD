// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "psm/MakePDNSim.hh"

#include <tcl.h>

#include "psm/pdnsim.h"
#include "utl/decode.h"

extern "C" {
extern int Psm_Init(Tcl_Interp* interp);
}

namespace psm {

extern const char* psm_tcl_inits[];

psm::PDNSim* makePDNSim()
{
  return new psm::PDNSim();
}

void initPDNSim(psm::PDNSim* pdnsim,
                utl::Logger* logger,
                odb::dbDatabase* db,
                sta::dbSta* sta,
                est::EstimateParasitics* estimate_parasitics,
                dpl::Opendp* opendp,
                Tcl_Interp* tcl_interp)
{
  Psm_Init(tcl_interp);
  utl::evalTclInit(tcl_interp, psm::psm_tcl_inits);
  pdnsim->init(logger, db, sta, estimate_parasitics, opendp);
}

void deletePDNSim(psm::PDNSim* pdnsim)
{
  delete pdnsim;
}

}  // namespace psm
