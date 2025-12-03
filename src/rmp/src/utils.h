// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <random>

#include "aig/gia/giaAig.h"
#include "base/abc/abc.h"
#include "db_sta/dbSta.hh"
#include "resynthesis_strategy.h"
#include "rsz/Resizer.hh"
#include "sta/Corner.hh"
#include "sta/Delay.hh"
#include "utl/Logger.h"
#include "utl/deleter.h"

namespace rmp {

utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> WrapUnique(abc::Abc_Ntk_t* ntk);
utl::UniquePtrWithDeleter<abc::Aig_Man_t> WrapUnique(abc::Aig_Man_t* aig);
utl::UniquePtrWithDeleter<abc::Gia_Man_t> WrapUnique(abc::Gia_Man_t* gia);

std::vector<sta::Vertex*> GetEndpoints(sta::dbSta* sta,
                                       rsz::Resizer* resizer,
                                       sta::Slack slack_threshold);

}  // namespace rmp
