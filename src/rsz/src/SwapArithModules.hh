// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <unordered_set>
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

using odb::dbModInst;
using sta::Path;
using std::unordered_set;
using utl::Logger;

class SwapArithModules : public sta::dbStaState
{
 public:
  explicit SwapArithModules(Resizer* resizer) : resizer_(resizer) {}
  virtual ~SwapArithModules() override = default;

  virtual void replaceArithModules(int path_count,
                                   const std::string& target,
                                   float slack_threshold = 0.0)
      = 0;
  virtual void collectArithInstsOnPath(Path* path, unordered_set<dbModInst*>& arithInsts)
      = 0;
  virtual bool isArithInstance(Instance* inst, dbModInst*& mod_inst) = 0;
  virtual bool hasArithOperatorProperty(dbModInst* mod_inst) = 0;
  virtual void findCriticalInstances(int path_count,
                                     const std::string& target,
                                     float slack_threshold,
                                     unordered_set<dbModInst*>& insts)
      = 0;
  virtual void doSwapInstances(const unordered_set<dbModInst*>& insts,
                               const std::string& target)
      = 0;

 protected:
  virtual void init() = 0;
  virtual void produceNewModuleName(const std::string& old_name,
                                    std::string& new_name,
                                    const std::string& target)
      = 0;

  // Member variables
  Resizer* resizer_;
  dbNetwork* db_network_;
  Logger* logger_;
  const MinMax* min_ = MinMax::min();
  const MinMax* max_ = MinMax::max();
};

}  // namespace rsz
