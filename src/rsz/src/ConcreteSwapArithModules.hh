// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <set>
#include <string>

#include "SwapArithModules.hh"
#include "odb/db.h"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"

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
  void collectArithInstsOnPath(const sta::Path* path,
                               std::set<odb::dbModInst*>& arithInsts) override;
  bool isArithInstance(const sta::Instance* inst,
                       odb::dbModInst*& mod_inst) override;
  bool hasArithOperatorProperty(const odb::dbModInst* mod_inst) override;
  void findCriticalInstances(int path_count,
                             const std::string& target,
                             float slack_threshold,
                             std::set<odb::dbModInst*>& insts) override;

  ///
  /// Swaps modules in the provided set to match the target.
  /// @param insts set of module instances to swap (input) or
  ///              new module instances after the swap (output)
  /// @param target optimization target (e.g., "setup")
  /// @return true if any instance was swapped
  ///
  bool doSwapInstances(std::set<odb::dbModInst*>& insts,
                       const std::string& target) override;

 protected:
  void init() override;
  void produceNewModuleName(const std::string& old_name,
                            std::string& new_name,
                            const std::string& target) override;

  bool init_ = false;
};

}  // namespace rsz
