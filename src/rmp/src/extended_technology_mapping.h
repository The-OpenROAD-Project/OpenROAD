// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"
#include "sta/Scene.hh"
#include "utl/Logger.h"

namespace rmp {

void extended_technology_mapping(sta::dbSta* sta,
                                 odb::dbDatabase* db,
                                 sta::Scene* corner,
                                 bool map_multioutput,
                                 bool area_oriented_mapping,
                                 bool verbose,
                                 rsz::Resizer* resizer,
                                 utl::Logger* logger);

}  // namespace rmp
