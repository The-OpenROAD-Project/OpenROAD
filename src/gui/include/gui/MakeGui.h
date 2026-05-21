// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "src/dbSta/include/db_sta/dbSta.hh"
#include "src/odb/include/odb/db.h"
#include "src/utl/include/utl/Logger.h"
#include "tcl.h"

namespace gui {

// There is no make/delete GUI as it is created at startup and can't
// be deleted.

void initGui(Tcl_Interp* interp,
             odb::dbDatabase* db,
             sta::dbSta* sta,
             utl::Logger* logger);

}  // namespace gui
