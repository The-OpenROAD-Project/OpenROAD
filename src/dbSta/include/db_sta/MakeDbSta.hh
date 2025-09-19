// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <tcl.h>

namespace utl {
class Logger;
}

namespace odb {
class dbDatabase;
}

namespace sta {

class dbSta;

sta::dbSta* makeDbSta();
void deleteDbSta(sta::dbSta* sta);
void initDbSta(sta::dbSta* sta,
               utl::Logger* logger,
               Tcl_Interp* tcl_interp,
               odb::dbDatabase* db);

}  // namespace sta
