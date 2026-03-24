// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2026, The OpenROAD Authors

#pragma once

#include <vector>

#include "aig/aig/aig.h"
#include "aig/gia/gia.h"
#include "base/abc/abc.h"
#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "utl/deleter.h"

namespace rsz {
class Resizer;
}  // namespace rsz

namespace sta {
class dbSta;
}  // namespace sta

namespace rmp {

utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> WrapUnique(abc::Abc_Ntk_t* ntk);
utl::UniquePtrWithDeleter<abc::Aig_Man_t> WrapUnique(abc::Aig_Man_t* aig);
utl::UniquePtrWithDeleter<abc::Gia_Man_t> WrapUnique(abc::Gia_Man_t* gia);

std::vector<sta::Vertex*> GetEndpoints(sta::dbSta* sta,
                                       rsz::Resizer* resizer,
                                       sta::Slack slack_threshold);

}  // namespace rmp
