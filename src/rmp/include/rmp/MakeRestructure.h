// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <tcl.h>

namespace utl {
class Logger;
}

namespace sta {
class dbSta;
}

namespace odb {
class dbDatabase;
}

namespace rsz {
class Resizer;
}

namespace rmp {

class Restructure;

rmp::Restructure* makeRestructure();

void initRestructure(rmp::Restructure* restructure,
                     utl::Logger* logger,
                     sta::dbSta* sta,
                     odb::dbDatabase* db,
                     rsz::Resizer* resizer,
                     Tcl_Interp* tcl_interp);

void deleteRestructure(rmp::Restructure* restructure);

}  // namespace rmp
