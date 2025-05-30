// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <set>
#include <string>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"
#include "sta/MinMax.hh"
#include "sta/StaState.hh"
#include "utl/Logger.h"

namespace odb {
class dbDatabase;
class dbInst;
}  // namespace odb

namespace sta {
class dbSta;
class LibertyCell;
class PathEnd;
class Pin;
class PinSet;
}  // namespace sta

namespace utl {
class Logger;
}

namespace rsz {

class Resizer;

using odb::dbDatabase;
using odb::dbInst;
using odb::dbModInst;
using sta::dbSta;
using sta::LibertyCell;
using sta::Path;
using sta::PathEnd;
using sta::Pin;
using sta::PinSet;
using std::set;
using std::string;
using utl::Logger;

class SwapArithModules : public sta::dbStaState
{
 public:
  explicit SwapArithModules(Resizer* resizer);
  ~SwapArithModules() = default;

  // Main entry point for arithmetic module replacement
  // path_count: Number of critical paths to analyze
  // target: Optimization target ("setup", "hold", "power", "area")
  // slack_threshold: Only consider paths with slack worse than this value
  void replaceArithModules(int path_count,
                           const string& target,
                           float slack_threshold = 0.0);
  void collectArithInstsOnPath(Path* path, set<dbModInst*>& arithInsts);
  bool isArithInstance(Instance* inst, dbModInst*& mod_inst);
  bool hasArithOperatorProperty(dbModInst* mod_inst);
  void doSwapInstances(set<dbModInst*> insts, const string& target);

 private:
  void init();
  void produceNewModuleName(const string& old_name,
                            string& new_name,
                            const string& target);

  // Member variables
  Resizer* resizer_;
  dbNetwork* db_network_;
  Logger* logger_;
  const MinMax* min_ = MinMax::min();
  const MinMax* max_ = MinMax::max();
};

}  // namespace rsz
