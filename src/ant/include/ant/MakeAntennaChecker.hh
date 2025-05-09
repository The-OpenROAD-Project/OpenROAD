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

namespace ant {

class AntennaChecker;
class GlobalRouteSource;

AntennaChecker* makeAntennaChecker();

void deleteAntennaChecker(ant::AntennaChecker* antennachecker);

void initAntennaChecker(ant::AntennaChecker* antenna_checker,
                        odb::dbDatabase* db,
                        ant::GlobalRouteSource* global_route_source,
                        utl::Logger* logger,
                        Tcl_Interp* tcl_interp);

}  // namespace ant
