// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include "SwapArithModules.hh"

namespace rsz {

class Resizer;

using odb::dbModInst;
using sta::Path;

class ConcreteSwapArithModules : public SwapArithModules
{
 public:
  explicit ConcreteSwapArithModules(Resizer* resizer);
  ~ConcreteSwapArithModules() override = default;

  void replaceArithModules(int path_count,
                           const std::string& target,
                           float slack_threshold) override;
  void collectArithInstsOnPath(Path* path,
                               unordered_set<dbModInst*>& arithInsts) override;
  bool isArithInstance(Instance* inst, dbModInst*& mod_inst) override;
  bool hasArithOperatorProperty(dbModInst* mod_inst) override;
  void findCriticalInstances(int path_count,
                             const std::string& target,
                             float slack_threshold,
                             unordered_set<dbModInst*>& insts) override;
  void doSwapInstances(const unordered_set<dbModInst*>& insts,
                       const std::string& target) override;

 protected:
  void init() override;
  void produceNewModuleName(const std::string& old_name,
                            std::string& new_name,
                            const std::string& target) override;
};

}  // namespace rsz
