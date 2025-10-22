// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include "SwapArithModules.hh"

namespace rsz {

class Resizer;

class ConcreteSwapArithModules : public SwapArithModules
{
 public:
  explicit ConcreteSwapArithModules(Resizer* resizer);
  ~ConcreteSwapArithModules() override = default;

  bool replaceArithModules(int path_count,
                           const std::string& target,
                           float slack_threshold) override;
  void collectArithInstsOnPath(const Path* path,
                               std::set<dbModInst*>& arithInsts) override;
  bool isArithInstance(const Instance* inst, dbModInst*& mod_inst) override;
  bool hasArithOperatorProperty(const dbModInst* mod_inst) override;
  void findCriticalInstances(int path_count,
                             const std::string& target,
                             float slack_threshold,
                             std::set<dbModInst*>& insts) override;
  bool doSwapInstances(const std::set<dbModInst*>& insts,
                       const std::string& target) override;

 protected:
  void init() override;
  void produceNewModuleName(const std::string& old_name,
                            std::string& new_name,
                            const std::string& target) override;

  bool init_ = false;
};

}  // namespace rsz
