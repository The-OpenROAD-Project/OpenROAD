// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include <tcl.h>

namespace odb {
class dbDatabase;
}

namespace sta {
class dbSta;
}

namespace utl {
class Logger;
}

namespace dft {
class Dft;

Dft* makeDft();
void initDft(dft::Dft* dft,
             odb::dbDatabase* db,
             sta::dbSta* sta,
             utl::Logger* logger,
             Tcl_Interp* tcl_interp);
void deleteDft(Dft* dft);

}  // namespace dft
