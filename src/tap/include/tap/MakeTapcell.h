// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <tcl.h>

namespace odb {
class dbDatabase;
}

namespace utl {
class Logger;
}

namespace tap {
class Tapcell;

tap::Tapcell* makeTapcell();

void deleteTapcell(tap::Tapcell* tapcell);

void initTapcell(tap::Tapcell* tapcell,
                 odb::dbDatabase* db,
                 utl::Logger* logger,
                 Tcl_Interp* tcl_interp);

}  // namespace tap
