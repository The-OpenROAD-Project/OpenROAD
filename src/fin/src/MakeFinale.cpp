// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "fin/MakeFinale.h"

#include <tcl.h>

#include "fin/Finale.h"
#include "utl/decode.h"

extern "C" {
extern int Fin_Init(Tcl_Interp* interp);
}

namespace fin {

// Tcl files encoded into strings.
extern const char* fin_tcl_inits[];

fin::Finale* makeFinale()
{
  return new fin::Finale;
}

void deleteFinale(fin::Finale* finale)
{
  delete finale;
}

void initFinale(fin::Finale* finale,
                odb::dbDatabase* db,
                utl::Logger* logger,
                Tcl_Interp* tcl_interp)
{
  // Define swig TCL commands.
  Fin_Init(tcl_interp);
  // Eval encoded sta TCL sources.
  utl::evalTclInit(tcl_interp, fin::fin_tcl_inits);
  finale->init(db, logger);
}

}  // namespace fin
