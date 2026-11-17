// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <set>
#include <string>
#include <vector>

#include "SwapArithModules.hh"
#include "odb/PtrSetMap.h"
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
  void collectArithInstsOnPath(
      const sta::Path* path,
      odb::PtrSet<odb::dbModInst>& arithInsts) override;
  bool isArithInstance(const sta::Instance* inst,
                       odb::dbModInst*& mod_inst) override;
  bool hasArithOperatorProperty(const odb::dbModInst* mod_inst) override;
  void findCriticalInstances(int path_count,
                             const std::string& target,
                             float slack_threshold,
                             odb::PtrSet<odb::dbModInst>& insts) override;

  ///
  /// Swaps modules in the provided set to match the target.
  /// @param insts set of module instances to swap (input) or
  ///              new module instances after the swap (output)
  /// @param target optimization target (e.g., "setup")
  /// @return true if any instance was swapped
  ///
  bool doSwapInstances(odb::PtrSet<odb::dbModInst>& insts,
                       const std::string& target) override;

 protected:
  void init() override;
  void produceNewModuleName(const std::string& old_name,
                            std::string& new_name,
                            const std::string& target) override;

  // Removes ports of inst's module that new_master lacks when each of them
  // only carries a buffered copy of a signal already crossing a port
  // new_master has: one entering through an input, or one of the module's
  // own outputs brought back in (e.g. ports punched by hierarchical
  // buffering).
  // Returns the number of ports removed, or -1 if any such port carries a
  // different signal or the remaining ports would not match new_master's,
  // in which case nothing is changed. The names of the buffers that drove
  // removed ports are appended to port_drivers.
  int foldDuplicatePorts(odb::dbModInst* inst,
                         odb::dbModule* new_master,
                         std::vector<std::string>& port_drivers);
  // Removes the named buffers, and the buffers feeding them, while they
  // drive no loads. Names that no longer exist are skipped.
  void removeUnloadedBuffers(const std::vector<std::string>& buffers);
  // Follows net upstream through buffers to the net of the first
  // non-buffer driver.
  odb::dbNet* bufferRoot(odb::dbNet* net);

  bool init_ = false;
};

}  // namespace rsz
