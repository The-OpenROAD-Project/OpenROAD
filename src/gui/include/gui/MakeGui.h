// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include <tcl.h>

#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "utl/Logger.h"

namespace ord {

// There is no make/delete GUI as it is created at startup and can't
// be deleted.

void initGui(Tcl_Interp* interp,
             odb::dbDatabase* db,
             sta::dbSta* sta,
             utl::Logger* logger);

}  // namespace ord
