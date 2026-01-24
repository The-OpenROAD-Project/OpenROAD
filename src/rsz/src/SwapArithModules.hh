// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <set>
#include <string>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "rsz/Resizer.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
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

namespace rsz {

class Resizer;

class SwapArithModules : public sta::dbStaState
{
 public:
  explicit SwapArithModules(Resizer* resizer) : resizer_(resizer) {}
  ~SwapArithModules() override = default;

  virtual bool replaceArithModules(int path_count,
                                   const std::string& target,
                                   float slack_threshold)
      = 0;
  virtual void collectArithInstsOnPath(const sta::Path* path,
                                       std::set<odb::dbModInst*>& arithInsts)
      = 0;
  virtual bool isArithInstance(const sta::Instance* inst,
                               odb::dbModInst*& mod_inst)
      = 0;
  virtual bool hasArithOperatorProperty(const odb::dbModInst* mod_inst) = 0;
  virtual void findCriticalInstances(int path_count,
                                     const std::string& target,
                                     float slack_threshold,
                                     std::set<odb::dbModInst*>& insts)
      = 0;
  virtual bool doSwapInstances(std::set<odb::dbModInst*>& insts,
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
  sta::dbNetwork* db_network_{nullptr};
  utl::Logger* logger_{nullptr};
  const sta::MinMax* min_ = sta::MinMax::min();
  const sta::MinMax* max_ = sta::MinMax::max();
};

}  // namespace rsz
