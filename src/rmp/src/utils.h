// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <random>
#include <vector>

#include "base/abc/abc.h"
#include "db_sta/dbSta.hh"
#include "resynthesis_strategy.h"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "utl/Logger.h"
#include "utl/deleter.h"

namespace rmp {

utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> WrapUnique(abc::Abc_Ntk_t* ntk);

std::vector<sta::Vertex*> GetEndpoints(sta::dbSta* sta,
                                       rsz::Resizer* resizer,
                                       sta::Slack slack_threshold);

}  // namespace rmp
