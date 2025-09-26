// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "ram/MakeRam.h"

#include <tcl.h>

#include "ram/ram.h"
#include "sta/StaMain.hh"
#include "utl/decode.h"


extern "C" {
extern int Ram_Init(Tcl_Interp* interp);
}

namespace ram {

// Tcl files encoded into strings.
extern const char* ram_tcl_inits[];

ram::RamGen* makeRamGen()
{
  return new ram::RamGen;
}

void deleteRamGen(ram::RamGen* ram_gen)
{
  delete ram_gen;
}

void initRamGen (ram::RamGen* ram_gen,
		sta::dbNetwork* network,
		odb::dbDatabase* db,
		utl::Logger* logger,
		Tcl_Interp* tcl_interp)
{
  // Define swig TCL commands.
  Ram_Init(tcl_interp);
  // Eval encoded sta TCL sources.
  utl::evalTclInit(tcl_interp, ram_tcl_inits);
  ram_gen->init(db, network, logger);
}

}  // namespace ram
