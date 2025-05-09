// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "ppl/MakeIoplacer.h"

#include "ppl/IOPlacer.h"
#include "utl/decode.h"

extern "C" {
extern int Ppl_Init(Tcl_Interp* interp);
}

namespace ppl {

// Tcl files encoded into strings.
extern const char* ppl_tcl_inits[];

ppl::IOPlacer* makeIoplacer()
{
  return new ppl::IOPlacer();
}

void deleteIoplacer(ppl::IOPlacer* ioplacer)
{
  delete ioplacer;
}

void initIoplacer(ppl::IOPlacer* placer,
                  odb::dbDatabase* db,
                  utl::Logger* logger,
                  Tcl_Interp* tcl_interp)
{
  // Define swig TCL commands.
  Ppl_Init(tcl_interp);
  utl::evalTclInit(tcl_interp, ppl::ppl_tcl_inits);

  placer->init(db, logger);
}

}  // namespace ppl
