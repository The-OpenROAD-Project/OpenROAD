// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

namespace sta {
class dbSta;
}

namespace utl {
class Logger;
}

namespace odb {
class dbDatabase;
}

struct Tcl_Interp;

namespace ord {

sta::dbSta* makeDbSta();
void deleteDbSta(sta::dbSta* sta);
void initDbSta(sta::dbSta* sta,
               utl::Logger* logger,
               Tcl_Interp* tcl_interp,
               odb::dbDatabase* db);

}  // namespace ord
