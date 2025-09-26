// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <tcl.h>

namespace sta {
class dbNetwork;
}

namespace odb {
class dbDatabase;
}

namespace utl {
class Logger;
}


namespace ram {

class RamGen;

ram::RamGen* makeRamGen();
void initRamGen(ram::RamGen* ram_gen,
		sta::dbNetwork* network,
		odb::dbDatabase* db,
		utl::Logger* logger,
		Tcl_Interp* tcl_interp);
void deleteRamGen(ram::RamGen* ram);

}  // namespace ram
