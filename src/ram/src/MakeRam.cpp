// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "ram/MakeRam.h"

#include <tcl.h>

#include "ord/OpenRoad.hh"
#include "ram/ram.h"
#include "sta/StaMain.hh"

namespace sta {
// Tcl files encoded into strings.
extern const char* ram_tcl_inits[];
}  // namespace sta

extern "C" {
extern int Ram_Init(Tcl_Interp* interp);
}

namespace ord {

ram::RamGen* makeRamGen()
{
  return new ram::RamGen;
}

void deleteRamGen(ram::RamGen* ram_gen)
{
  delete ram_gen;
}

void initRamGen(OpenRoad* openroad)
{
  Tcl_Interp* tcl_interp = openroad->tclInterp();
  // Define swig TCL commands.
  Ram_Init(tcl_interp);
  // Eval encoded sta TCL sources.
  sta::evalTclInit(tcl_interp, sta::ram_tcl_inits);
  openroad->getRamGen()->init(
      openroad->getDb(), openroad->getDbNetwork(), openroad->getLogger());
}

}  // namespace ord
