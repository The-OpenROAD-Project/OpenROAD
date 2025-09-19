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

using odb::dbModInst;
using sta::Instance;
using sta::Path;
using utl::Logger;

class SwapArithModules : public sta::dbStaState
{
 public:
  explicit SwapArithModules(Resizer* resizer) : resizer_(resizer) {}
  ~SwapArithModules() override = default;

  virtual void replaceArithModules(int path_count,
                                   const std::string& target,
                                   float slack_threshold)
      = 0;
  virtual void collectArithInstsOnPath(const Path* path,
                                       std::set<dbModInst*>& arithInsts)
      = 0;
  virtual bool isArithInstance(const Instance* inst, dbModInst*& mod_inst) = 0;
  virtual bool hasArithOperatorProperty(const dbModInst* mod_inst) = 0;
  virtual void findCriticalInstances(int path_count,
                                     const std::string& target,
                                     float slack_threshold,
                                     std::set<dbModInst*>& insts)
      = 0;
  virtual void doSwapInstances(const std::set<dbModInst*>& insts,
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
  dbNetwork* db_network_{nullptr};
  Logger* logger_{nullptr};
  const MinMax* min_ = MinMax::min();
  const MinMax* max_ = MinMax::max();
};

}  // namespace rsz
