// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#pragma once

#include <tcl.h>

namespace odb {
class dbDatabase;
}
namespace sta {
class dbSta;
}
namespace rsz {
class Resizer;
}
namespace grt {
class GlobalRouter;
}
namespace utl {
class Logger;
}

namespace gpl {

class Replace;

gpl::Replace* makeReplace();

void initReplace(gpl::Replace* replace,
                 odb::dbDatabase* db,
                 sta::dbSta* sta,
                 rsz::Resizer* resizer,
                 grt::GlobalRouter* global_route,
                 utl::Logger* logger,
                 Tcl_Interp* tcl_interp);

void deleteReplace(gpl::Replace* replace);

}  // namespace gpl
