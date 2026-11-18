// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "tcl.h"

namespace odb {
class dbDatabase;
}

namespace utl {
class Logger;
}  // namespace utl

namespace grt {

class GlobalRouter;

// Does GUI dependency injection
void initGui(grt::GlobalRouter* grt, odb::dbDatabase* db, utl::Logger* logger);

void initTcl(Tcl_Interp* tcl_interp);

}  // namespace grt
