// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include "SwapArithModules.hh"

namespace rsz {

class Resizer;

using odb::dbModInst;
using sta::Path;
using std::set;
using utl::Logger;

class ConcreteSwapArithModules : public SwapArithModules
{
 public:
  explicit ConcreteSwapArithModules(Resizer* resizer);
  ~ConcreteSwapArithModules() override = default;

  void replaceArithModules(int path_count,
                           const std::string& target,
                           float slack_threshold = 0.0);
  void collectArithInstsOnPath(Path* path,
                               unordered_set<dbModInst*>& arithInsts);
  bool isArithInstance(Instance* inst, dbModInst*& mod_inst);
  bool hasArithOperatorProperty(dbModInst* mod_inst);
  void findCriticalInstances(int path_count,
                             const std::string& target,
                             float slack_threshold,
                             unordered_set<dbModInst*>& insts);
  void doSwapInstances(const unordered_set<dbModInst*>& insts,
                       const std::string& target);

 protected:
  void init();
  void produceNewModuleName(const std::string& old_name,
                            std::string& new_name,
                            const std::string& target);
};

}  // namespace rsz
