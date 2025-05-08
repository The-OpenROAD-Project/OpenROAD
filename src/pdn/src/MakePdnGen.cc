// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "pdn/MakePdnGen.hh"

#include <tcl.h>

#include "domain.h"
#include "grid.h"
#include "pdn/PdnGen.hh"
#include "power_cells.h"
#include "renderer.h"
#include "utl/decode.h"

extern "C" {
extern int Pdn_Init(Tcl_Interp* interp);
}

namespace pdn {

extern const char* pdn_tcl_inits[];

void initPdnGen(pdn::PdnGen* pdngen,
                odb::dbDatabase* db,
                utl::Logger* logger,
                Tcl_Interp* tcl_interp)
{
  // Define swig TCL commands.
  Pdn_Init(tcl_interp);
  // Eval encoded sta TCL sources.
  utl::evalTclInit(tcl_interp, pdn::pdn_tcl_inits);

  pdngen->init(db, logger);
}

pdn::PdnGen* makePdnGen()
{
  return new pdn::PdnGen();
}

void deletePdnGen(pdn::PdnGen* pdngen)
{
  delete pdngen;
}

}  // namespace pdn
