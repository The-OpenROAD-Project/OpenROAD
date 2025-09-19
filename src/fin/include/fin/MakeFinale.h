// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include <tcl.h>

namespace odb {
class dbDatabase;
}

namespace utl {
class Logger;
}

namespace fin {

class Finale;

fin::Finale* makeFinale();
void initFinale(fin::Finale* finale,
                odb::dbDatabase* db,
                utl::Logger* logger,
                Tcl_Interp* tcl_interp);
void deleteFinale(fin::Finale* finale);

}  // namespace fin
