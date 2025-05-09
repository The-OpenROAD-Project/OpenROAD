// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "ant/MakeAntennaChecker.hh"

#include "ant/AntennaChecker.hh"
#include "grt/GlobalRouter.h"
#include "utl/decode.h"

extern "C" {
extern int Ant_Init(Tcl_Interp* interp);
}

namespace ant {

// Tcl files encoded into strings.
extern const char* ant_tcl_inits[];

ant::AntennaChecker* makeAntennaChecker()
{
  return new ant::AntennaChecker;
}

void deleteAntennaChecker(ant::AntennaChecker* antenna_checker)
{
  delete antenna_checker;
}

void initAntennaChecker(ant::AntennaChecker* antenna_checker,
                        odb::dbDatabase* db,
                        ant::GlobalRouteSource* global_route_source,
                        utl::Logger* logger,
                        Tcl_Interp* tcl_interp)
{
  Ant_Init(tcl_interp);
  utl::evalTclInit(tcl_interp, ant::ant_tcl_inits);
  antenna_checker->init(db, global_route_source, logger);
}

}  // namespace ant
